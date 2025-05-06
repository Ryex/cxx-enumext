use std::collections::HashSet;
use std::fmt::Display;

use proc_macro::TokenStream;
use proc_macro2::Span;
use quote::{quote, quote_spanned, ToTokens};
use syn::ext::IdentExt;
use syn::{
    parse::{Parse, ParseStream, Parser},
    spanned::Spanned,
};
use syn::{
    Attribute, Fields, GenericArgument, GenericParam, Generics, Ident, Item as RustItem, ItemEnum,
    ItemType, Lit, LitStr, Path, PathArguments, Token, Type, Variant, Visibility,
};

use syn::{Error as SynError, Result as SynResult};

struct Errors {
    errors: Vec<SynError>,
}

impl Errors {
    pub fn new() -> Self {
        Errors { errors: Vec::new() }
    }

    #[allow(dead_code)]
    pub fn error(&mut self, sp: impl ToTokens, msg: impl Display) {
        self.errors.push(SynError::new_spanned(sp, msg));
    }

    pub fn push(&mut self, error: SynError) {
        self.errors.push(error);
    }

    pub fn propagate(&mut self) -> SynResult<()> {
        let mut iter = self.errors.drain(..);
        let Some(mut all_errors) = iter.next() else {
            return Ok(());
        };
        for err in iter {
            all_errors.combine(err);
        }
        Err(all_errors)
    }
}

impl Default for Errors {
    fn default() -> Self {
        Self::new()
    }
}

#[proc_macro_attribute]
pub fn extern_type(attribute: TokenStream, input: TokenStream) -> TokenStream {
    match AstPieces::from_token_streams(attribute, input) {
        Err(error) => error.into_compile_error().into(),
        Ok(pieces) => expand(pieces).into(),
    }
}

fn expand(pieces: AstPieces) -> proc_macro2::TokenStream {
    let mut output = proc_macro2::TokenStream::new();

    let ident = &pieces.ident;
    let qualified_name = Lit::Str(LitStr::new(
        &format!(
            "{}{}",
            pieces
                .namespace
                .clone()
                .unwrap_or_default()
                .0
                .iter()
                .fold(String::new(), |acc, piece| acc + piece + "::"),
            pieces
                .cxx_name
                .clone()
                .unwrap_or_else(|| ForeignName(ident.to_string()))
                .0
        ),
        ident.span(),
    ));

    output.extend(match &pieces.item {
        Item::Enum(enm) => expand_enum(&pieces, enm),
        Item::Optional(optional) => expand_optional(&pieces, optional),
        Item::Expected(expected) => expand_expected(&pieces, expected),
    });

    let cfg = &pieces.cfg;
    let generics = &pieces.generics;

    output.extend(quote! {

        #cfg
        #[automatically_derived]
        unsafe impl #generics ::cxx::ExternType for #ident #generics {
            #[allow(unused_attributes)] // incorrect lint
            #[doc(hidden)]
            type Id = ::cxx::type_id!(#qualified_name);
            type Kind = ::cxx::kind::Trivial;
        }

    });

    output.extend(expand_asserts(&pieces));

    output
}

fn expand_enum(pieces: &AstPieces, enm: &Enum) -> proc_macro2::TokenStream {
    let ident = &pieces.ident;
    let vis = &pieces.vis;
    let attrs = pieces.attrs.iter();
    let generics = &pieces.generics;
    let cfg = &pieces.cfg;
    let variants = enm.variants.iter().map(|variant| {
        let attrs = variant.attrs.iter();
        let ident = &variant.ident;
        match &variant.fields {
            Fields::Named(named) => {
                let list = named.named.iter();
                quote! {#(#attrs)* #ident { #(#list),* } }
            }
            Fields::Unnamed(unnamed) => {
                let list = unnamed.unnamed.iter();
                quote! {#(#attrs)* #ident ( #(#list),* ) }
            }
            Fields::Unit => {
                quote! {#(#attrs)* #ident }
            }
        }
    });

    quote! {
        #cfg
        #(#attrs)*
        #[repr(C)]
        #vis enum #ident #generics {
            #(#variants,)*
        }
    }
}

fn expand_optional(pieces: &AstPieces, optional: &Optional) -> proc_macro2::TokenStream {
    let ident = &pieces.ident;
    let vis = &pieces.vis;
    let attrs = pieces.attrs.iter();
    let generics = &pieces.generics;
    let inner = &optional.inner;
    let cfg = &pieces.cfg;

    quote! {
        #cfg
        #(#attrs)*
        #[repr(C)]
        #vis enum #ident #generics {
            None,
            Some(#inner)
        }

        #cfg
        #[automatically_derived]
        impl #generics ::std::convert::From<#ident #generics> for Option<#inner> {
            fn from(value: #ident) -> Self {
                match value {
                    #ident::None => None,
                    #ident::Some(value) => Some(value),
                }
            }
        }

        #cfg
        #[automatically_derived]
        impl #generics ::std::convert::From<Option<#inner>> for #ident #generics{
            fn from(value: Option<#inner>) -> Self {
                match value {
                    None => #ident::None,
                    Some(value) => #ident::Some(value),
                }
            }
        }
    }
}

fn expand_expected(pieces: &AstPieces, expected: &Expected) -> proc_macro2::TokenStream {
    let ident = &pieces.ident;
    let vis = &pieces.vis;
    let attrs = pieces.attrs.iter();
    let generics = &pieces.generics;
    let cfg = &pieces.cfg;
    let expected_t = &expected.expected;
    let unexpected_t = &expected.unexpected;

    quote! {
        #cfg
        #(#attrs)*
        #[repr(C)]
        #vis enum #ident #generics {
            Ok(#expected_t),
            Err(#unexpected_t),
        }

        #cfg
        #[automatically_derived]
        impl #generics ::std::convert::From<#ident #generics> for Result<#expected_t, #unexpected_t> {
            fn from(value: #ident) -> Self {
                match value {
                    #ident::Ok(value) => Ok(value),
                    #ident::Err(value) => Err(value),
                }
            }
        }

        #cfg
        #[automatically_derived]
        impl #generics ::std::convert::From<Result<#expected_t, #unexpected_t>> for #ident #generics {
            fn from(value: Result<#expected_t, #unexpected_t>) -> Self {
                match value {
                    Ok(value) => #ident::Ok(value),
                    Err(value) => #ident::Err(value),
                }
            }
        }
    }
}

fn expand_asserts(pieces: &AstPieces) -> proc_macro2::TokenStream {
    let mut seen_trivial = HashSet::new();
    let mut seen_opaque = HashSet::new();
    let mut seen_extern = HashSet::new();
    let mut seen_box = HashSet::new();
    let mut seen_vec = HashSet::new();

    let mut verify_extern = proc_macro2::TokenStream::new();

    let mut assert_extern =
        |span: Span, path: &Path, verify_extern: &mut proc_macro2::TokenStream| {
            if !seen_extern.contains(path) {
                seen_extern.insert(path.clone());
                let reason = format!(" {} Must be ::cxx::ExternType", path.to_token_stream());
                let assert = quote_spanned! {span=> assert!(::cxx_enumext::private::IsCxxExternType::<#path>::IS_CXX_EXTERN_TYPE,#reason )};
                verify_extern.extend(quote! {
                    const _: () = #assert;
                });
            }
        };

    for ty in pieces.extern_types.iter() {
        match ty {
            ExternType::Trivial(path) => {
                let span = path.span();
                assert_extern(span, path, &mut verify_extern);
                if !seen_trivial.contains(path) {
                    seen_trivial.insert(path.clone());
                    let reason = format!(
                        " {} Must be ::cxx::ExternType<Kind = Trivial>",
                        path.to_token_stream()
                    );
                    let assert = quote_spanned! {span=> assert!(::cxx_enumext::private::IsCxxExternTrivial::<#path>::IS_CXX_EXTERN_TRIVIAL, #reason)};
                    verify_extern.extend(quote! {
                        const _: () = #assert;
                    });
                }
            }
            ExternType::Opaque(path) => {
                let span = path.span();
                assert_extern(span, path, &mut verify_extern);
                if !seen_opaque.contains(path) {
                    seen_opaque.insert(path.clone());
                    let reason = format!(
                        " {} Must be ::cxx::ExternType<Kind = Opaque>",
                        path.to_token_stream()
                    );
                    let assert = quote_spanned! {span=> assert!(::cxx_enumext::private::IsCxxExternOpaque::<#path>::IS_CXX_EXTERN_OPAQUE, #reason)};
                    verify_extern.extend(quote! {
                        const _: () = #assert;
                    });
                }
            }
            ExternType::Unspecified(path) => {
                assert_extern(path.span(), path, &mut verify_extern);
            }
        }
    }

    let mut verify_box = proc_macro2::TokenStream::new();

    for path in pieces.box_types.iter() {
        if !seen_box.contains(path) {
            seen_box.insert(path.clone());
            let span = path.span();
            let reason = format!("{} is not exposed inside a Box in a cxx bridge. Use a Box as a function parameter/return value or shared struct member", path.to_token_stream());
            let assert = quote_spanned!(span=> assert!(cxx_enumext::private::IsCxxImplBox::<#path>::IS_CXX_IMPL_BOX, #reason));
            verify_box.extend(quote! {
                const _: () = #assert;
            });
        }
    }

    for path in pieces.vec_types.iter() {
        if !seen_vec.contains(path) {
            seen_vec.insert(path.clone());
            let span = path.span();
            let reason = format!("{} is not exposed inside a Vec in a cxx bridge. Use a Vec as a function parameter/return value or shared struct member", path.to_token_stream());
            let assert = quote_spanned!(span=> assert!(cxx_enumext::private::IsCxxImplBox::<#path>::IS_CXX_IMPL_VEC, #reason));
            verify_box.extend(quote! {
                const _: () = #assert;
            });
        }
    }

    let cfg = &pieces.cfg;

    quote! {
        #cfg
        #[doc(hidden)]
        const _: () = {
            use ::cxx_enumext::private::NotCxxExternType as _;
            use ::cxx_enumext::private::NotCxxExternTrivial as _;
            use ::cxx_enumext::private::NotCxxExternOpaque as _;
            use ::cxx_enumext::private::NotCxxImplBox as _;
            use ::cxx_enumext::private::NotCxxImplVec as _;

            #verify_extern
            #verify_box
            #verify_extern
        };
    }
}

struct Enum {
    variants: Vec<Variant>,
}

struct Optional {
    inner: Type,
}

struct Expected {
    expected: Type,
    unexpected: Type,
}

enum Item {
    Enum(Enum),
    Optional(Optional),
    Expected(Expected),
}

enum ExternType {
    Trivial(Path),
    Opaque(Path),
    #[allow(dead_code)]
    Unspecified(Path),
}

struct AstPieces {
    /// the item to bridge
    item: Item,
    ident: Ident,
    namespace: Option<Namespace>,
    cxx_name: Option<ForeignName>,
    vis: Visibility,
    generics: Generics,
    attrs: Vec<Attribute>,
    cfg: Option<Attribute>,
    /// type name to confirm impl cxx::private::ImplBox
    box_types: Vec<Path>,
    /// type name to confirm impl cxx::private::ImplVec
    vec_types: Vec<Path>,
    /// type name and kind of cxx::ExternType to confirm
    extern_types: Vec<ExternType>,
}

mod kw {
    syn::custom_keyword!(namespace);
    syn::custom_keyword!(cxx_name);
}

#[derive(Default, Clone)]
struct Namespace(pub Vec<String>);

#[derive(Default, Clone)]
struct ForeignName(pub String);

impl ForeignName {
    pub fn parse(text: &str, span: Span) -> SynResult<Self> {
        match Ident::parse_any.parse_str(text) {
            Ok(ident) => {
                let text = ident.to_string();
                Ok(ForeignName(text))
            }
            Err(err) => Err(SynError::new(span, err)),
        }
    }
}

impl Parse for Namespace {
    fn parse(input: ParseStream) -> SynResult<Self> {
        if input.is_empty() {
            return Ok(Namespace(vec![]));
        }
        let path = input.call(Path::parse_mod_style)?;
        Ok(Namespace(
            path.segments
                .iter()
                .map(|segment| segment.ident.to_string())
                .collect(),
        ))
    }
}

fn parse_bridge_params(input: ParseStream) -> SynResult<(Option<Namespace>, Option<ForeignName>)> {
    if input.is_empty() {
        Ok((None, None))
    } else {
        let mut ns = None;
        let mut cxx_name = None;
        loop {
            if input.peek(kw::namespace) {
                let ns_tok = input.parse::<kw::namespace>()?;
                if ns.is_some() {
                    return Err(SynError::new_spanned(ns_tok, "duplicate namespace param"));
                }
                input.parse::<Token![=]>()?;
                ns = Some(input.parse::<Namespace>()?);
            } else if input.peek(kw::cxx_name) {
                let name_tok = input.parse::<kw::cxx_name>()?;
                if cxx_name.is_some() {
                    return Err(SynError::new_spanned(name_tok, "duplicate cxx_name param"));
                }
                input.parse::<Token![=]>()?;
                cxx_name = Some(ForeignName::parse(
                    &input.parse::<LitStr>()?.value(),
                    name_tok.span,
                )?);
            }

            if (input.parse::<Option<Token![,]>>()?).is_none() {
                break;
            }
        }
        Ok((ns, cxx_name))
    }
}

impl AstPieces {
    // Parses the macro arguments and returns the pieces, returning a `syn::Error` on error.
    fn from_token_streams(attribute: TokenStream, item: TokenStream) -> SynResult<AstPieces> {
        let (namespace, cxx_name) = parse_bridge_params.parse(attribute)?;

        match syn::parse::<RustItem>(item)? {
            RustItem::Type(ty) => parse_type_decl(ty, namespace, cxx_name),
            RustItem::Enum(enm) => parse_enum(enm, namespace, cxx_name),
            other => Err(SynError::new_spanned(
                other,
                "unsupported item for ExternType generation",
            )),
        }
    }
}

fn parse_enum(
    enm: ItemEnum,
    namespace: Option<Namespace>,
    cxx_name: Option<ForeignName>,
) -> SynResult<AstPieces> {
    let cx = &mut Errors::new();

    let mut box_types = Vec::new();
    let mut vec_types = Vec::new();
    let mut extern_types = Vec::new();

    let mut attrs = enm.attrs;
    let mut cfg = None;
    attrs.retain_mut(|attr| {
        let attr_path = attr.path();
        if attr_path.is_ident("cfg") {
            cfg = Some(attr.clone());
            return false;
        }
        if attr_path.is_ident("repr") {
            cx.push(SynError::new_spanned(attr, "unsupported repr attribute"));
        }
        true
    });

    for variant in &enm.variants {
        match &variant.fields {
            Fields::Named(named) => {
                for field in &named.named {
                    find_types(
                        &field.ty,
                        &mut box_types,
                        &mut vec_types,
                        &mut extern_types,
                        cx,
                    );
                }
            }
            Fields::Unit => {}
            Fields::Unnamed(unnamed) => {
                for field in &unnamed.unnamed {
                    find_types(
                        &field.ty,
                        &mut box_types,
                        &mut vec_types,
                        &mut extern_types,
                        cx,
                    );
                }
            }
        }
    }
    for generic in &enm.generics.params {
        if !matches!(generic, GenericParam::Lifetime(_)) {
            cx.push(SynError::new_spanned(
                generic,
                "only lifetime generic params supported",
            ));
        }
    }
    cx.propagate()?;
    Ok(AstPieces {
        item: Item::Enum(Enum {
            variants: enm.variants.into_iter().collect(),
        }),
        ident: enm.ident.clone(),
        cxx_name,
        namespace,
        attrs,
        vis: enm.vis,
        generics: enm.generics,
        cfg,
        box_types,
        vec_types,
        extern_types,
    })
}

fn parse_type_decl(
    alias: ItemType,
    namespace: Option<Namespace>,
    cxx_name: Option<ForeignName>,
) -> SynResult<AstPieces> {
    let cx = &mut Errors::new();
    let ident = alias.ident;
    if !alias.generics.params.is_empty() {
        cx.push(SynError::new_spanned(
            alias.generics.params.clone(),
            "Generics are not supported",
        ));
    }

    let mut box_types = Vec::new();
    let mut vec_types = Vec::new();
    let mut extern_types = Vec::new();

    let mut attrs = alias.attrs;
    let mut cfg = None;
    attrs.retain_mut(|attr| {
        let attr_path = attr.path();
        if attr_path.is_ident("cfg") {
            cfg = Some(attr.clone());
            return false;
        }
        if attr_path.is_ident("repr") {
            cx.push(SynError::new_spanned(attr, "unsupported repr attribute"));
        }
        true
    });

    match alias.ty.as_ref() {
        Type::Path(ty) => {
            let path = &ty.path;
            if ty.qself.is_none() {
                let segment = {
                    if path.segments.len() == 1 {
                        &path.segments[0]
                    } else if path.segments.len() == 2 && path.segments[0].ident == "cxx_enumext" {
                        &path.segments[1]
                    } else {
                        return Err(SynError::new_spanned(
                            path,
                            "unsupported type, did you mean 'Optional' or 'cxx_enumext::Optional'?",
                        ));
                    }
                };
                let ty_ident = segment.ident.clone();
                if ty_ident == "Option" {
                    return Err(SynError::new_spanned(
                        path,
                        "unsupported type, did you mean 'Optional'?",
                    ));
                } else if ty_ident == "Result" {
                    return Err(SynError::new_spanned(
                        path,
                        "unsupported type, did you mean 'Expected'?",
                    ));
                } else if ty_ident == "Optional" {
                    let inner = match &segment.arguments {
                        PathArguments::None => {
                            return Err(SynError::new_spanned(
                                path,
                                "Optional needs a contained type",
                            ));
                        }
                        PathArguments::Parenthesized(_) => {
                            return Err(SynError::new_spanned(
                                path,
                                "Optional needs a contained type",
                            ));
                        }
                        PathArguments::AngleBracketed(generic) => {
                            if generic.args.len() == 1 {
                                let GenericArgument::Type(inner) = &generic.args[0] else {
                                    return Err(SynError::new_spanned(
                                        path,
                                        "Optional takes only one generic type argument",
                                    ));
                                };

                                find_types(
                                    inner,
                                    &mut box_types,
                                    &mut vec_types,
                                    &mut extern_types,
                                    cx,
                                );
                                inner
                            } else {
                                return Err(SynError::new_spanned(
                                    path,
                                    "Optional takes only one generic type argument",
                                ));
                            }
                        }
                    };
                    cx.propagate()?;
                    return Ok(AstPieces {
                        item: Item::Optional(Optional {
                            inner: inner.clone(),
                        }),
                        ident,
                        namespace,
                        cxx_name,
                        attrs,
                        vis: alias.vis,
                        generics: alias.generics,
                        cfg,
                        box_types,
                        vec_types,
                        extern_types,
                    });
                } else if ty_ident == "Expected" {
                    let (expected, unexpected) = match &segment.arguments {
                        PathArguments::None => {
                            return Err(SynError::new_spanned(
                                path,
                                "Expected needs two contained types",
                            ));
                        }
                        PathArguments::Parenthesized(_) => {
                            return Err(SynError::new_spanned(
                                path,
                                "Expected needs two contained types",
                            ));
                        }
                        PathArguments::AngleBracketed(generic) => {
                            if generic.args.len() == 2 {
                                let GenericArgument::Type(expected) = &generic.args[0] else {
                                    return Err(SynError::new_spanned(
                                        &generic.args[0],
                                        "must be a type argument",
                                    ));
                                };
                                let GenericArgument::Type(unexpected) = &generic.args[1] else {
                                    return Err(SynError::new_spanned(
                                        &generic.args[0],
                                        "must be a type argument",
                                    ));
                                };

                                find_types(
                                    expected,
                                    &mut box_types,
                                    &mut vec_types,
                                    &mut extern_types,
                                    cx,
                                );
                                find_types(
                                    unexpected,
                                    &mut box_types,
                                    &mut vec_types,
                                    &mut extern_types,
                                    cx,
                                );
                                (expected, unexpected)
                            } else {
                                return Err(SynError::new_spanned(
                                    path,
                                    "Expected takes two generic type argument",
                                ));
                            }
                        }
                    };
                    cx.propagate()?;
                    return Ok(AstPieces {
                        item: Item::Expected(Expected {
                            expected: expected.clone(),
                            unexpected: unexpected.clone(),
                        }),
                        ident,
                        namespace,
                        cxx_name,
                        attrs,
                        vis: alias.vis,
                        generics: alias.generics,
                        cfg,
                        box_types,
                        vec_types,
                        extern_types,
                    });
                };
            }
            Err(SynError::new_spanned(path, "unsupported type"))
        }
        other => Err(SynError::new_spanned(other, "unsupported type")),
    }
}

fn find_types(
    ty: &Type,
    box_types: &mut Vec<Path>,
    vec_types: &mut Vec<Path>,
    extern_types: &mut Vec<ExternType>,
    cx: &mut Errors,
) {
    match ty {
        Type::Reference(_) => {}
        Type::Ptr(_) => {}
        Type::Array(_) => {}
        Type::BareFn(_) => {}
        Type::Tuple(ty) if ty.elems.is_empty() => {}
        Type::Path(ty) => {
            let path = &ty.path;

            // used to detect if type was handeled already
            let count = extern_types.len() + box_types.len() + vec_types.len();

            if ty.qself.is_none() && path.leading_colon.is_none() && path.segments.len() == 1 {
                let segment = &path.segments[0];
                let ident = segment.ident.clone();
                match &segment.arguments {
                    PathArguments::None => {}
                    PathArguments::AngleBracketed(generic) => {
                        if ident == "Box" && generic.args.len() == 1 {
                            if let GenericArgument::Type(Type::Path(inner)) = &generic.args[0] {
                                box_types.push(inner.path.clone());
                            }
                        } else if ident == "Vec" && generic.args.len() == 1 {
                            if let GenericArgument::Type(Type::Path(inner)) = &generic.args[0] {
                                vec_types.push(inner.path.clone());
                            }
                        } else if ["UniquePtr", "SharedPtr", "WeakPtr", "CxxVector"]
                            .iter()
                            .any(|name| ident == name)
                            && generic.args.len() == 1
                        {
                            if let GenericArgument::Type(Type::Path(inner)) = &generic.args[0] {
                                extern_types.push(ExternType::Opaque(inner.path.clone()));
                            }
                        }
                    }
                    PathArguments::Parenthesized(_) => {}
                }
            } else if ty.qself.is_none()
                && path.leading_colon.is_none()
                && path.segments.len() == 2
                && path.segments[0].ident == "cxx"
            {
                let segment = &path.segments[1];
                let ident = segment.ident.clone();

                match &segment.arguments {
                    PathArguments::None => {}
                    PathArguments::AngleBracketed(generic) => {
                        if ["UniquePtr", "SharedPtr", "WeakPtr", "CxxVector"]
                            .iter()
                            .any(|name| ident == name)
                            && generic.args.len() == 1
                        {
                            if let GenericArgument::Type(Type::Path(inner)) = &generic.args[0] {
                                extern_types.push(ExternType::Opaque(inner.path.clone()));
                            }
                        }
                    }
                    PathArguments::Parenthesized(_) => {}
                }
            }

            if (extern_types.len() + box_types.len() + vec_types.len()) == count {
                // didn't find a special type
                extern_types.push(ExternType::Trivial(path.clone()));
            }
        }
        other => {
            cx.push(SynError::new_spanned(other, "unsupported type"));
        }
    }
}
