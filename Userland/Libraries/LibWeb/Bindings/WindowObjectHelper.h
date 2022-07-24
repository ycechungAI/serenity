/*
 * Copyright (c) 2021-2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// FIXME: Find a way to generate all of this

#include <LibWeb/Bindings/AbortControllerConstructor.h>
#include <LibWeb/Bindings/AbortControllerPrototype.h>
#include <LibWeb/Bindings/AbortSignalConstructor.h>
#include <LibWeb/Bindings/AbortSignalPrototype.h>
#include <LibWeb/Bindings/AbstractRangeConstructor.h>
#include <LibWeb/Bindings/AbstractRangePrototype.h>
#include <LibWeb/Bindings/AudioConstructor.h>
#include <LibWeb/Bindings/BlobConstructor.h>
#include <LibWeb/Bindings/BlobPrototype.h>
#include <LibWeb/Bindings/CDATASectionConstructor.h>
#include <LibWeb/Bindings/CDATASectionPrototype.h>
#include <LibWeb/Bindings/CSSConditionRuleConstructor.h>
#include <LibWeb/Bindings/CSSConditionRulePrototype.h>
#include <LibWeb/Bindings/CSSFontFaceRuleConstructor.h>
#include <LibWeb/Bindings/CSSFontFaceRulePrototype.h>
#include <LibWeb/Bindings/CSSGroupingRuleConstructor.h>
#include <LibWeb/Bindings/CSSGroupingRulePrototype.h>
#include <LibWeb/Bindings/CSSImportRuleConstructor.h>
#include <LibWeb/Bindings/CSSImportRulePrototype.h>
#include <LibWeb/Bindings/CSSMediaRuleConstructor.h>
#include <LibWeb/Bindings/CSSMediaRulePrototype.h>
#include <LibWeb/Bindings/CSSRuleConstructor.h>
#include <LibWeb/Bindings/CSSRuleListConstructor.h>
#include <LibWeb/Bindings/CSSRuleListPrototype.h>
#include <LibWeb/Bindings/CSSRulePrototype.h>
#include <LibWeb/Bindings/CSSStyleDeclarationConstructor.h>
#include <LibWeb/Bindings/CSSStyleDeclarationPrototype.h>
#include <LibWeb/Bindings/CSSStyleRuleConstructor.h>
#include <LibWeb/Bindings/CSSStyleRulePrototype.h>
#include <LibWeb/Bindings/CSSStyleSheetConstructor.h>
#include <LibWeb/Bindings/CSSStyleSheetPrototype.h>
#include <LibWeb/Bindings/CSSSupportsRuleConstructor.h>
#include <LibWeb/Bindings/CSSSupportsRulePrototype.h>
#include <LibWeb/Bindings/CanvasGradientConstructor.h>
#include <LibWeb/Bindings/CanvasGradientPrototype.h>
#include <LibWeb/Bindings/CanvasRenderingContext2DConstructor.h>
#include <LibWeb/Bindings/CanvasRenderingContext2DPrototype.h>
#include <LibWeb/Bindings/CharacterDataConstructor.h>
#include <LibWeb/Bindings/CharacterDataPrototype.h>
#include <LibWeb/Bindings/CloseEventConstructor.h>
#include <LibWeb/Bindings/CloseEventPrototype.h>
#include <LibWeb/Bindings/CommentConstructor.h>
#include <LibWeb/Bindings/CommentPrototype.h>
#include <LibWeb/Bindings/CryptoConstructor.h>
#include <LibWeb/Bindings/CryptoPrototype.h>
#include <LibWeb/Bindings/CustomEventConstructor.h>
#include <LibWeb/Bindings/CustomEventPrototype.h>
#include <LibWeb/Bindings/DOMExceptionConstructor.h>
#include <LibWeb/Bindings/DOMExceptionPrototype.h>
#include <LibWeb/Bindings/DOMImplementationConstructor.h>
#include <LibWeb/Bindings/DOMImplementationPrototype.h>
#include <LibWeb/Bindings/DOMParserConstructor.h>
#include <LibWeb/Bindings/DOMParserPrototype.h>
#include <LibWeb/Bindings/DOMPointConstructor.h>
#include <LibWeb/Bindings/DOMPointPrototype.h>
#include <LibWeb/Bindings/DOMPointReadOnlyConstructor.h>
#include <LibWeb/Bindings/DOMPointReadOnlyPrototype.h>
#include <LibWeb/Bindings/DOMRectConstructor.h>
#include <LibWeb/Bindings/DOMRectListConstructor.h>
#include <LibWeb/Bindings/DOMRectListPrototype.h>
#include <LibWeb/Bindings/DOMRectPrototype.h>
#include <LibWeb/Bindings/DOMRectReadOnlyConstructor.h>
#include <LibWeb/Bindings/DOMRectReadOnlyPrototype.h>
#include <LibWeb/Bindings/DOMStringMapConstructor.h>
#include <LibWeb/Bindings/DOMStringMapPrototype.h>
#include <LibWeb/Bindings/DocumentConstructor.h>
#include <LibWeb/Bindings/DocumentFragmentConstructor.h>
#include <LibWeb/Bindings/DocumentFragmentPrototype.h>
#include <LibWeb/Bindings/DocumentPrototype.h>
#include <LibWeb/Bindings/DocumentTypeConstructor.h>
#include <LibWeb/Bindings/DocumentTypePrototype.h>
#include <LibWeb/Bindings/ElementConstructor.h>
#include <LibWeb/Bindings/ElementPrototype.h>
#include <LibWeb/Bindings/ErrorEventConstructor.h>
#include <LibWeb/Bindings/ErrorEventPrototype.h>
#include <LibWeb/Bindings/EventConstructor.h>
#include <LibWeb/Bindings/EventPrototype.h>
#include <LibWeb/Bindings/EventTargetConstructor.h>
#include <LibWeb/Bindings/EventTargetPrototype.h>
#include <LibWeb/Bindings/FileConstructor.h>
#include <LibWeb/Bindings/FilePrototype.h>
#include <LibWeb/Bindings/FocusEventConstructor.h>
#include <LibWeb/Bindings/FocusEventPrototype.h>
#include <LibWeb/Bindings/HTMLAnchorElementConstructor.h>
#include <LibWeb/Bindings/HTMLAnchorElementPrototype.h>
#include <LibWeb/Bindings/HTMLAreaElementConstructor.h>
#include <LibWeb/Bindings/HTMLAreaElementPrototype.h>
#include <LibWeb/Bindings/HTMLAudioElementConstructor.h>
#include <LibWeb/Bindings/HTMLAudioElementPrototype.h>
#include <LibWeb/Bindings/HTMLBRElementConstructor.h>
#include <LibWeb/Bindings/HTMLBRElementPrototype.h>
#include <LibWeb/Bindings/HTMLBaseElementConstructor.h>
#include <LibWeb/Bindings/HTMLBaseElementPrototype.h>
#include <LibWeb/Bindings/HTMLBodyElementConstructor.h>
#include <LibWeb/Bindings/HTMLBodyElementPrototype.h>
#include <LibWeb/Bindings/HTMLButtonElementConstructor.h>
#include <LibWeb/Bindings/HTMLButtonElementPrototype.h>
#include <LibWeb/Bindings/HTMLCanvasElementConstructor.h>
#include <LibWeb/Bindings/HTMLCanvasElementPrototype.h>
#include <LibWeb/Bindings/HTMLCollectionConstructor.h>
#include <LibWeb/Bindings/HTMLCollectionPrototype.h>
#include <LibWeb/Bindings/HTMLDListElementConstructor.h>
#include <LibWeb/Bindings/HTMLDListElementPrototype.h>
#include <LibWeb/Bindings/HTMLDataElementConstructor.h>
#include <LibWeb/Bindings/HTMLDataElementPrototype.h>
#include <LibWeb/Bindings/HTMLDataListElementConstructor.h>
#include <LibWeb/Bindings/HTMLDataListElementPrototype.h>
#include <LibWeb/Bindings/HTMLDetailsElementConstructor.h>
#include <LibWeb/Bindings/HTMLDetailsElementPrototype.h>
#include <LibWeb/Bindings/HTMLDialogElementConstructor.h>
#include <LibWeb/Bindings/HTMLDialogElementPrototype.h>
#include <LibWeb/Bindings/HTMLDirectoryElementConstructor.h>
#include <LibWeb/Bindings/HTMLDirectoryElementPrototype.h>
#include <LibWeb/Bindings/HTMLDivElementConstructor.h>
#include <LibWeb/Bindings/HTMLDivElementPrototype.h>
#include <LibWeb/Bindings/HTMLElementConstructor.h>
#include <LibWeb/Bindings/HTMLElementPrototype.h>
#include <LibWeb/Bindings/HTMLEmbedElementConstructor.h>
#include <LibWeb/Bindings/HTMLEmbedElementPrototype.h>
#include <LibWeb/Bindings/HTMLFieldSetElementConstructor.h>
#include <LibWeb/Bindings/HTMLFieldSetElementPrototype.h>
#include <LibWeb/Bindings/HTMLFontElementConstructor.h>
#include <LibWeb/Bindings/HTMLFontElementPrototype.h>
#include <LibWeb/Bindings/HTMLFormElementConstructor.h>
#include <LibWeb/Bindings/HTMLFormElementPrototype.h>
#include <LibWeb/Bindings/HTMLFrameElementConstructor.h>
#include <LibWeb/Bindings/HTMLFrameElementPrototype.h>
#include <LibWeb/Bindings/HTMLFrameSetElementConstructor.h>
#include <LibWeb/Bindings/HTMLFrameSetElementPrototype.h>
#include <LibWeb/Bindings/HTMLHRElementConstructor.h>
#include <LibWeb/Bindings/HTMLHRElementPrototype.h>
#include <LibWeb/Bindings/HTMLHeadElementConstructor.h>
#include <LibWeb/Bindings/HTMLHeadElementPrototype.h>
#include <LibWeb/Bindings/HTMLHeadingElementConstructor.h>
#include <LibWeb/Bindings/HTMLHeadingElementPrototype.h>
#include <LibWeb/Bindings/HTMLHtmlElementConstructor.h>
#include <LibWeb/Bindings/HTMLHtmlElementPrototype.h>
#include <LibWeb/Bindings/HTMLIFrameElementConstructor.h>
#include <LibWeb/Bindings/HTMLIFrameElementPrototype.h>
#include <LibWeb/Bindings/HTMLImageElementConstructor.h>
#include <LibWeb/Bindings/HTMLImageElementPrototype.h>
#include <LibWeb/Bindings/HTMLInputElementConstructor.h>
#include <LibWeb/Bindings/HTMLInputElementPrototype.h>
#include <LibWeb/Bindings/HTMLLIElementConstructor.h>
#include <LibWeb/Bindings/HTMLLIElementPrototype.h>
#include <LibWeb/Bindings/HTMLLabelElementConstructor.h>
#include <LibWeb/Bindings/HTMLLabelElementPrototype.h>
#include <LibWeb/Bindings/HTMLLegendElementConstructor.h>
#include <LibWeb/Bindings/HTMLLegendElementPrototype.h>
#include <LibWeb/Bindings/HTMLLinkElementConstructor.h>
#include <LibWeb/Bindings/HTMLLinkElementPrototype.h>
#include <LibWeb/Bindings/HTMLMapElementConstructor.h>
#include <LibWeb/Bindings/HTMLMapElementPrototype.h>
#include <LibWeb/Bindings/HTMLMarqueeElementConstructor.h>
#include <LibWeb/Bindings/HTMLMarqueeElementPrototype.h>
#include <LibWeb/Bindings/HTMLMediaElementConstructor.h>
#include <LibWeb/Bindings/HTMLMediaElementPrototype.h>
#include <LibWeb/Bindings/HTMLMenuElementConstructor.h>
#include <LibWeb/Bindings/HTMLMenuElementPrototype.h>
#include <LibWeb/Bindings/HTMLMetaElementConstructor.h>
#include <LibWeb/Bindings/HTMLMetaElementPrototype.h>
#include <LibWeb/Bindings/HTMLMeterElementConstructor.h>
#include <LibWeb/Bindings/HTMLMeterElementPrototype.h>
#include <LibWeb/Bindings/HTMLModElementConstructor.h>
#include <LibWeb/Bindings/HTMLModElementPrototype.h>
#include <LibWeb/Bindings/HTMLOListElementConstructor.h>
#include <LibWeb/Bindings/HTMLOListElementPrototype.h>
#include <LibWeb/Bindings/HTMLObjectElementConstructor.h>
#include <LibWeb/Bindings/HTMLObjectElementPrototype.h>
#include <LibWeb/Bindings/HTMLOptGroupElementConstructor.h>
#include <LibWeb/Bindings/HTMLOptGroupElementPrototype.h>
#include <LibWeb/Bindings/HTMLOptionElementConstructor.h>
#include <LibWeb/Bindings/HTMLOptionElementPrototype.h>
#include <LibWeb/Bindings/HTMLOptionsCollectionConstructor.h>
#include <LibWeb/Bindings/HTMLOptionsCollectionPrototype.h>
#include <LibWeb/Bindings/HTMLOutputElementConstructor.h>
#include <LibWeb/Bindings/HTMLOutputElementPrototype.h>
#include <LibWeb/Bindings/HTMLParagraphElementConstructor.h>
#include <LibWeb/Bindings/HTMLParagraphElementPrototype.h>
#include <LibWeb/Bindings/HTMLParamElementConstructor.h>
#include <LibWeb/Bindings/HTMLParamElementPrototype.h>
#include <LibWeb/Bindings/HTMLPictureElementConstructor.h>
#include <LibWeb/Bindings/HTMLPictureElementPrototype.h>
#include <LibWeb/Bindings/HTMLPreElementConstructor.h>
#include <LibWeb/Bindings/HTMLPreElementPrototype.h>
#include <LibWeb/Bindings/HTMLProgressElementConstructor.h>
#include <LibWeb/Bindings/HTMLProgressElementPrototype.h>
#include <LibWeb/Bindings/HTMLQuoteElementConstructor.h>
#include <LibWeb/Bindings/HTMLQuoteElementPrototype.h>
#include <LibWeb/Bindings/HTMLScriptElementConstructor.h>
#include <LibWeb/Bindings/HTMLScriptElementPrototype.h>
#include <LibWeb/Bindings/HTMLSelectElementConstructor.h>
#include <LibWeb/Bindings/HTMLSelectElementPrototype.h>
#include <LibWeb/Bindings/HTMLSlotElementConstructor.h>
#include <LibWeb/Bindings/HTMLSlotElementPrototype.h>
#include <LibWeb/Bindings/HTMLSourceElementConstructor.h>
#include <LibWeb/Bindings/HTMLSourceElementPrototype.h>
#include <LibWeb/Bindings/HTMLSpanElementConstructor.h>
#include <LibWeb/Bindings/HTMLSpanElementPrototype.h>
#include <LibWeb/Bindings/HTMLStyleElementConstructor.h>
#include <LibWeb/Bindings/HTMLStyleElementPrototype.h>
#include <LibWeb/Bindings/HTMLTableCaptionElementConstructor.h>
#include <LibWeb/Bindings/HTMLTableCaptionElementPrototype.h>
#include <LibWeb/Bindings/HTMLTableCellElementConstructor.h>
#include <LibWeb/Bindings/HTMLTableCellElementPrototype.h>
#include <LibWeb/Bindings/HTMLTableColElementConstructor.h>
#include <LibWeb/Bindings/HTMLTableColElementPrototype.h>
#include <LibWeb/Bindings/HTMLTableElementConstructor.h>
#include <LibWeb/Bindings/HTMLTableElementPrototype.h>
#include <LibWeb/Bindings/HTMLTableRowElementConstructor.h>
#include <LibWeb/Bindings/HTMLTableRowElementPrototype.h>
#include <LibWeb/Bindings/HTMLTableSectionElementConstructor.h>
#include <LibWeb/Bindings/HTMLTableSectionElementPrototype.h>
#include <LibWeb/Bindings/HTMLTemplateElementConstructor.h>
#include <LibWeb/Bindings/HTMLTemplateElementPrototype.h>
#include <LibWeb/Bindings/HTMLTextAreaElementConstructor.h>
#include <LibWeb/Bindings/HTMLTextAreaElementPrototype.h>
#include <LibWeb/Bindings/HTMLTimeElementConstructor.h>
#include <LibWeb/Bindings/HTMLTimeElementPrototype.h>
#include <LibWeb/Bindings/HTMLTitleElementConstructor.h>
#include <LibWeb/Bindings/HTMLTitleElementPrototype.h>
#include <LibWeb/Bindings/HTMLTrackElementConstructor.h>
#include <LibWeb/Bindings/HTMLTrackElementPrototype.h>
#include <LibWeb/Bindings/HTMLUListElementConstructor.h>
#include <LibWeb/Bindings/HTMLUListElementPrototype.h>
#include <LibWeb/Bindings/HTMLUnknownElementConstructor.h>
#include <LibWeb/Bindings/HTMLUnknownElementPrototype.h>
#include <LibWeb/Bindings/HTMLVideoElementConstructor.h>
#include <LibWeb/Bindings/HTMLVideoElementPrototype.h>
#include <LibWeb/Bindings/HeadersConstructor.h>
#include <LibWeb/Bindings/HeadersPrototype.h>
#include <LibWeb/Bindings/HistoryConstructor.h>
#include <LibWeb/Bindings/HistoryPrototype.h>
#include <LibWeb/Bindings/IdleDeadlineConstructor.h>
#include <LibWeb/Bindings/IdleDeadlinePrototype.h>
#include <LibWeb/Bindings/ImageConstructor.h>
#include <LibWeb/Bindings/ImageDataConstructor.h>
#include <LibWeb/Bindings/ImageDataPrototype.h>
#include <LibWeb/Bindings/IntersectionObserverConstructor.h>
#include <LibWeb/Bindings/IntersectionObserverPrototype.h>
#include <LibWeb/Bindings/KeyboardEventConstructor.h>
#include <LibWeb/Bindings/KeyboardEventPrototype.h>
#include <LibWeb/Bindings/LocationConstructor.h>
#include <LibWeb/Bindings/LocationPrototype.h>
#include <LibWeb/Bindings/MediaListConstructor.h>
#include <LibWeb/Bindings/MediaListPrototype.h>
#include <LibWeb/Bindings/MediaQueryListConstructor.h>
#include <LibWeb/Bindings/MediaQueryListEventConstructor.h>
#include <LibWeb/Bindings/MediaQueryListEventPrototype.h>
#include <LibWeb/Bindings/MediaQueryListPrototype.h>
#include <LibWeb/Bindings/MessageChannelConstructor.h>
#include <LibWeb/Bindings/MessageChannelPrototype.h>
#include <LibWeb/Bindings/MessageEventConstructor.h>
#include <LibWeb/Bindings/MessageEventPrototype.h>
#include <LibWeb/Bindings/MouseEventConstructor.h>
#include <LibWeb/Bindings/MouseEventPrototype.h>
#include <LibWeb/Bindings/MutationObserverConstructor.h>
#include <LibWeb/Bindings/MutationObserverPrototype.h>
#include <LibWeb/Bindings/MutationRecordConstructor.h>
#include <LibWeb/Bindings/MutationRecordPrototype.h>
#include <LibWeb/Bindings/NavigatorConstructor.h>
#include <LibWeb/Bindings/NavigatorPrototype.h>
#include <LibWeb/Bindings/NodeConstructor.h>
#include <LibWeb/Bindings/NodeIteratorConstructor.h>
#include <LibWeb/Bindings/NodeIteratorPrototype.h>
#include <LibWeb/Bindings/NodeListConstructor.h>
#include <LibWeb/Bindings/NodeListPrototype.h>
#include <LibWeb/Bindings/NodePrototype.h>
#include <LibWeb/Bindings/OptionConstructor.h>
#include <LibWeb/Bindings/PageTransitionEventConstructor.h>
#include <LibWeb/Bindings/PageTransitionEventPrototype.h>
#include <LibWeb/Bindings/PerformanceConstructor.h>
#include <LibWeb/Bindings/PerformancePrototype.h>
#include <LibWeb/Bindings/PerformanceTimingConstructor.h>
#include <LibWeb/Bindings/PerformanceTimingPrototype.h>
#include <LibWeb/Bindings/ProcessingInstructionConstructor.h>
#include <LibWeb/Bindings/ProcessingInstructionPrototype.h>
#include <LibWeb/Bindings/ProgressEventConstructor.h>
#include <LibWeb/Bindings/ProgressEventPrototype.h>
#include <LibWeb/Bindings/PromiseRejectionEventConstructor.h>
#include <LibWeb/Bindings/PromiseRejectionEventPrototype.h>
#include <LibWeb/Bindings/RangeConstructor.h>
#include <LibWeb/Bindings/RangePrototype.h>
#include <LibWeb/Bindings/ResizeObserverConstructor.h>
#include <LibWeb/Bindings/ResizeObserverPrototype.h>
#include <LibWeb/Bindings/SVGCircleElementConstructor.h>
#include <LibWeb/Bindings/SVGCircleElementPrototype.h>
#include <LibWeb/Bindings/SVGClipPathElementConstructor.h>
#include <LibWeb/Bindings/SVGClipPathElementPrototype.h>
#include <LibWeb/Bindings/SVGDefsElementConstructor.h>
#include <LibWeb/Bindings/SVGDefsElementPrototype.h>
#include <LibWeb/Bindings/SVGElementConstructor.h>
#include <LibWeb/Bindings/SVGElementPrototype.h>
#include <LibWeb/Bindings/SVGEllipseElementConstructor.h>
#include <LibWeb/Bindings/SVGEllipseElementPrototype.h>
#include <LibWeb/Bindings/SVGGeometryElementConstructor.h>
#include <LibWeb/Bindings/SVGGeometryElementPrototype.h>
#include <LibWeb/Bindings/SVGGraphicsElementConstructor.h>
#include <LibWeb/Bindings/SVGGraphicsElementPrototype.h>
#include <LibWeb/Bindings/SVGLineElementConstructor.h>
#include <LibWeb/Bindings/SVGLineElementPrototype.h>
#include <LibWeb/Bindings/SVGPathElementConstructor.h>
#include <LibWeb/Bindings/SVGPathElementPrototype.h>
#include <LibWeb/Bindings/SVGPolygonElementConstructor.h>
#include <LibWeb/Bindings/SVGPolygonElementPrototype.h>
#include <LibWeb/Bindings/SVGPolylineElementConstructor.h>
#include <LibWeb/Bindings/SVGPolylineElementPrototype.h>
#include <LibWeb/Bindings/SVGRectElementConstructor.h>
#include <LibWeb/Bindings/SVGRectElementPrototype.h>
#include <LibWeb/Bindings/SVGSVGElementConstructor.h>
#include <LibWeb/Bindings/SVGSVGElementPrototype.h>
#include <LibWeb/Bindings/SVGTextContentElementConstructor.h>
#include <LibWeb/Bindings/SVGTextContentElementPrototype.h>
#include <LibWeb/Bindings/ScreenConstructor.h>
#include <LibWeb/Bindings/ScreenPrototype.h>
#include <LibWeb/Bindings/SelectionConstructor.h>
#include <LibWeb/Bindings/SelectionPrototype.h>
#include <LibWeb/Bindings/ShadowRootConstructor.h>
#include <LibWeb/Bindings/ShadowRootPrototype.h>
#include <LibWeb/Bindings/StaticRangeConstructor.h>
#include <LibWeb/Bindings/StaticRangePrototype.h>
#include <LibWeb/Bindings/StorageConstructor.h>
#include <LibWeb/Bindings/StoragePrototype.h>
#include <LibWeb/Bindings/StyleSheetConstructor.h>
#include <LibWeb/Bindings/StyleSheetListConstructor.h>
#include <LibWeb/Bindings/StyleSheetListPrototype.h>
#include <LibWeb/Bindings/StyleSheetPrototype.h>
#include <LibWeb/Bindings/SubmitEventConstructor.h>
#include <LibWeb/Bindings/SubmitEventPrototype.h>
#include <LibWeb/Bindings/SubtleCryptoConstructor.h>
#include <LibWeb/Bindings/SubtleCryptoPrototype.h>
#include <LibWeb/Bindings/TextConstructor.h>
#include <LibWeb/Bindings/TextDecoderConstructor.h>
#include <LibWeb/Bindings/TextDecoderPrototype.h>
#include <LibWeb/Bindings/TextEncoderConstructor.h>
#include <LibWeb/Bindings/TextEncoderPrototype.h>
#include <LibWeb/Bindings/TextMetricsConstructor.h>
#include <LibWeb/Bindings/TextMetricsPrototype.h>
#include <LibWeb/Bindings/TextPrototype.h>
#include <LibWeb/Bindings/TreeWalkerConstructor.h>
#include <LibWeb/Bindings/TreeWalkerPrototype.h>
#include <LibWeb/Bindings/UIEventConstructor.h>
#include <LibWeb/Bindings/UIEventPrototype.h>
#include <LibWeb/Bindings/URLConstructor.h>
#include <LibWeb/Bindings/URLPrototype.h>
#include <LibWeb/Bindings/URLSearchParamsConstructor.h>
#include <LibWeb/Bindings/URLSearchParamsPrototype.h>
#include <LibWeb/Bindings/WebGLContextEventConstructor.h>
#include <LibWeb/Bindings/WebGLContextEventPrototype.h>
#include <LibWeb/Bindings/WebGLRenderingContextConstructor.h>
#include <LibWeb/Bindings/WebGLRenderingContextPrototype.h>
#include <LibWeb/Bindings/WebSocketConstructor.h>
#include <LibWeb/Bindings/WebSocketPrototype.h>
#include <LibWeb/Bindings/WindowConstructor.h>
#include <LibWeb/Bindings/WindowPrototype.h>
#include <LibWeb/Bindings/WorkerConstructor.h>
#include <LibWeb/Bindings/WorkerPrototype.h>
#include <LibWeb/Bindings/XMLHttpRequestConstructor.h>
#include <LibWeb/Bindings/XMLHttpRequestEventTargetConstructor.h>
#include <LibWeb/Bindings/XMLHttpRequestEventTargetPrototype.h>
#include <LibWeb/Bindings/XMLHttpRequestPrototype.h>
#include <LibWeb/Bindings/XMLSerializerConstructor.h>
#include <LibWeb/Bindings/XMLSerializerPrototype.h>

#define ADD_WINDOW_OBJECT_CONSTRUCTOR_AND_PROTOTYPE(interface_name, constructor_name, prototype_name)                                \
    {                                                                                                                                \
        auto& constructor = ensure_web_constructor<constructor_name>(#interface_name);                                               \
        constructor.define_direct_property(vm.names.name, js_string(vm, #interface_name), JS::Attribute::Configurable);              \
        auto& prototype = ensure_web_prototype<prototype_name>(#interface_name);                                                     \
        prototype.define_direct_property(vm.names.constructor, &constructor, JS::Attribute::Writable | JS::Attribute::Configurable); \
    }

#define ADD_WINDOW_OBJECT_INTERFACE(interface_name) \
    ADD_WINDOW_OBJECT_CONSTRUCTOR_AND_PROTOTYPE(interface_name, interface_name##Constructor, interface_name##Prototype)

#define ADD_WINDOW_OBJECT_INTERFACES                                                                \
    auto& vm = this->vm();                                                                          \
    ADD_WINDOW_OBJECT_INTERFACE(AbortController)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(AbortSignal)                                                        \
    ADD_WINDOW_OBJECT_INTERFACE(AbstractRange)                                                      \
    ADD_WINDOW_OBJECT_INTERFACE(Blob)                                                               \
    ADD_WINDOW_OBJECT_INTERFACE(CDATASection)                                                       \
    ADD_WINDOW_OBJECT_INTERFACE(CSSConditionRule)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(CSSFontFaceRule)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(CSSGroupingRule)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(CSSImportRule)                                                      \
    ADD_WINDOW_OBJECT_INTERFACE(CSSMediaRule)                                                       \
    ADD_WINDOW_OBJECT_INTERFACE(CSSRule)                                                            \
    ADD_WINDOW_OBJECT_INTERFACE(CSSRuleList)                                                        \
    ADD_WINDOW_OBJECT_INTERFACE(CSSStyleDeclaration)                                                \
    ADD_WINDOW_OBJECT_INTERFACE(CSSStyleRule)                                                       \
    ADD_WINDOW_OBJECT_INTERFACE(CSSStyleSheet)                                                      \
    ADD_WINDOW_OBJECT_INTERFACE(CSSSupportsRule)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(CanvasGradient)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(CanvasRenderingContext2D)                                           \
    ADD_WINDOW_OBJECT_INTERFACE(CharacterData)                                                      \
    ADD_WINDOW_OBJECT_INTERFACE(CloseEvent)                                                         \
    ADD_WINDOW_OBJECT_INTERFACE(Comment)                                                            \
    ADD_WINDOW_OBJECT_INTERFACE(Crypto)                                                             \
    ADD_WINDOW_OBJECT_INTERFACE(CustomEvent)                                                        \
    ADD_WINDOW_OBJECT_INTERFACE(DocumentFragment)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(Document)                                                           \
    ADD_WINDOW_OBJECT_INTERFACE(DocumentType)                                                       \
    ADD_WINDOW_OBJECT_INTERFACE(DOMException)                                                       \
    ADD_WINDOW_OBJECT_INTERFACE(DOMImplementation)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(DOMParser)                                                          \
    ADD_WINDOW_OBJECT_INTERFACE(DOMPoint)                                                           \
    ADD_WINDOW_OBJECT_INTERFACE(DOMPointReadOnly)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(DOMRect)                                                            \
    ADD_WINDOW_OBJECT_INTERFACE(DOMRectList)                                                        \
    ADD_WINDOW_OBJECT_INTERFACE(DOMRectReadOnly)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(DOMStringMap)                                                       \
    ADD_WINDOW_OBJECT_INTERFACE(Element)                                                            \
    ADD_WINDOW_OBJECT_INTERFACE(ErrorEvent)                                                         \
    ADD_WINDOW_OBJECT_INTERFACE(Event)                                                              \
    ADD_WINDOW_OBJECT_INTERFACE(EventTarget)                                                        \
    ADD_WINDOW_OBJECT_INTERFACE(File)                                                               \
    ADD_WINDOW_OBJECT_INTERFACE(Headers)                                                            \
    ADD_WINDOW_OBJECT_INTERFACE(History)                                                            \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLAnchorElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLAreaElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLAudioElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLBaseElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLBodyElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLBRElement)                                                      \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLButtonElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLCanvasElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLCollection)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLDataElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLDataListElement)                                                \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLDetailsElement)                                                 \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLDialogElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLDirectoryElement)                                               \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLDivElement)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLDListElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLElement)                                                        \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLEmbedElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLFieldSetElement)                                                \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLFontElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLFormElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLFrameElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLFrameSetElement)                                                \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLHeadElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLHeadingElement)                                                 \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLHRElement)                                                      \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLHtmlElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLIFrameElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLImageElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLInputElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLLabelElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLLegendElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLLIElement)                                                      \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLLinkElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLMapElement)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLMarqueeElement)                                                 \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLMediaElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLMenuElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLMetaElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLMeterElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLModElement)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLObjectElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLOListElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLOptGroupElement)                                                \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLOptionElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLOptionsCollection)                                              \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLOutputElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLParagraphElement)                                               \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLParamElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLPictureElement)                                                 \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLPreElement)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLProgressElement)                                                \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLQuoteElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLScriptElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLSelectElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLSlotElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLSourceElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLSpanElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLStyleElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLTableCaptionElement)                                            \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLTableCellElement)                                               \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLTableColElement)                                                \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLTableElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLTableRowElement)                                                \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLTableSectionElement)                                            \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLTemplateElement)                                                \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLTextAreaElement)                                                \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLTimeElement)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLTitleElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLTrackElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLUListElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLUnknownElement)                                                 \
    ADD_WINDOW_OBJECT_INTERFACE(HTMLVideoElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(IdleDeadline)                                                       \
    ADD_WINDOW_OBJECT_INTERFACE(ImageData)                                                          \
    ADD_WINDOW_OBJECT_INTERFACE(IntersectionObserver)                                               \
    ADD_WINDOW_OBJECT_INTERFACE(KeyboardEvent)                                                      \
    ADD_WINDOW_OBJECT_INTERFACE(Location)                                                           \
    ADD_WINDOW_OBJECT_INTERFACE(MediaQueryList)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(MediaQueryListEvent)                                                \
    ADD_WINDOW_OBJECT_INTERFACE(MediaList)                                                          \
    ADD_WINDOW_OBJECT_INTERFACE(MessageChannel)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(MessageEvent)                                                       \
    ADD_WINDOW_OBJECT_INTERFACE(MouseEvent)                                                         \
    ADD_WINDOW_OBJECT_INTERFACE(MutationObserver)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(MutationRecord)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(Navigator)                                                          \
    ADD_WINDOW_OBJECT_INTERFACE(Node)                                                               \
    ADD_WINDOW_OBJECT_INTERFACE(NodeIterator)                                                       \
    ADD_WINDOW_OBJECT_INTERFACE(NodeList)                                                           \
    ADD_WINDOW_OBJECT_INTERFACE(PageTransitionEvent)                                                \
    ADD_WINDOW_OBJECT_INTERFACE(Performance)                                                        \
    ADD_WINDOW_OBJECT_INTERFACE(PerformanceTiming)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(ProcessingInstruction)                                              \
    ADD_WINDOW_OBJECT_INTERFACE(ProgressEvent)                                                      \
    ADD_WINDOW_OBJECT_INTERFACE(PromiseRejectionEvent)                                              \
    ADD_WINDOW_OBJECT_INTERFACE(Range)                                                              \
    ADD_WINDOW_OBJECT_INTERFACE(ResizeObserver)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(Screen)                                                             \
    ADD_WINDOW_OBJECT_INTERFACE(Selection)                                                          \
    ADD_WINDOW_OBJECT_INTERFACE(ShadowRoot)                                                         \
    ADD_WINDOW_OBJECT_INTERFACE(StaticRange)                                                        \
    ADD_WINDOW_OBJECT_INTERFACE(Storage)                                                            \
    ADD_WINDOW_OBJECT_INTERFACE(StyleSheet)                                                         \
    ADD_WINDOW_OBJECT_INTERFACE(StyleSheetList)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(SubmitEvent)                                                        \
    ADD_WINDOW_OBJECT_INTERFACE(SubtleCrypto)                                                       \
    ADD_WINDOW_OBJECT_INTERFACE(SVGElement)                                                         \
    ADD_WINDOW_OBJECT_INTERFACE(SVGCircleElement)                                                   \
    ADD_WINDOW_OBJECT_INTERFACE(SVGClipPathElement)                                                 \
    ADD_WINDOW_OBJECT_INTERFACE(SVGDefsElement)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(SVGEllipseElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(SVGGeometryElement)                                                 \
    ADD_WINDOW_OBJECT_INTERFACE(SVGGraphicsElement)                                                 \
    ADD_WINDOW_OBJECT_INTERFACE(SVGLineElement)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(SVGPathElement)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(SVGPolygonElement)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(SVGPolylineElement)                                                 \
    ADD_WINDOW_OBJECT_INTERFACE(SVGRectElement)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(SVGSVGElement)                                                      \
    ADD_WINDOW_OBJECT_INTERFACE(SVGTextContentElement)                                              \
    ADD_WINDOW_OBJECT_INTERFACE(Text)                                                               \
    ADD_WINDOW_OBJECT_INTERFACE(TextDecoder)                                                        \
    ADD_WINDOW_OBJECT_INTERFACE(TextEncoder)                                                        \
    ADD_WINDOW_OBJECT_INTERFACE(TextMetrics)                                                        \
    ADD_WINDOW_OBJECT_INTERFACE(TreeWalker)                                                         \
    ADD_WINDOW_OBJECT_INTERFACE(UIEvent)                                                            \
    ADD_WINDOW_OBJECT_INTERFACE(URLSearchParams)                                                    \
    ADD_WINDOW_OBJECT_INTERFACE(URL)                                                                \
    ADD_WINDOW_OBJECT_INTERFACE(WebGLContextEvent)                                                  \
    ADD_WINDOW_OBJECT_INTERFACE(WebGLRenderingContext)                                              \
    ADD_WINDOW_OBJECT_INTERFACE(WebSocket)                                                          \
    ADD_WINDOW_OBJECT_INTERFACE(Worker)                                                             \
    ADD_WINDOW_OBJECT_INTERFACE(XMLHttpRequest)                                                     \
    ADD_WINDOW_OBJECT_INTERFACE(XMLHttpRequestEventTarget)                                          \
    ADD_WINDOW_OBJECT_INTERFACE(XMLSerializer)                                                      \
    ADD_WINDOW_OBJECT_INTERFACE(Window)                                                             \
    ADD_WINDOW_OBJECT_CONSTRUCTOR_AND_PROTOTYPE(Audio, AudioConstructor, HTMLAudioElementPrototype) \
    ADD_WINDOW_OBJECT_CONSTRUCTOR_AND_PROTOTYPE(Image, ImageConstructor, HTMLImageElementPrototype) \
    ADD_WINDOW_OBJECT_CONSTRUCTOR_AND_PROTOTYPE(Option, OptionConstructor, HTMLOptionElementPrototype)
