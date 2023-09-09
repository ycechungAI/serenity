/*
 * Copyright (c) 2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/CSS/Display.h>

namespace Web::CSS {

String Display::to_string() const
{
    StringBuilder builder;
    switch (m_type) {
    case Type::OutsideAndInside:
        // NOTE: Following the precedence rules of “most backwards-compatible, then shortest”,
        //       serialization of equivalent display values uses the “Short display” column.
        if (*this == Display::from_short(Display::Short::Block))
            return "block"_string;
        if (*this == Display::from_short(Display::Short::FlowRoot))
            return "flow-root"_string;
        if (*this == Display::from_short(Display::Short::Inline))
            return "inline"_string;
        if (*this == Display::from_short(Display::Short::InlineBlock))
            return "inline-block"_string;
        if (*this == Display::from_short(Display::Short::RunIn))
            return "run-in"_string;
        if (*this == Display::from_short(Display::Short::ListItem))
            return "list-item"_string;
        if (*this == Display::from_short(Display::Short::Flex))
            return "flex"_string;
        if (*this == Display::from_short(Display::Short::InlineFlex))
            return "inline-flex"_string;
        if (*this == Display::from_short(Display::Short::Grid))
            return "grid"_string;
        if (*this == Display::from_short(Display::Short::InlineGrid))
            return "inline-grid"_string;
        if (*this == Display::from_short(Display::Short::Ruby))
            return "ruby"_string;
        if (*this == Display::from_short(Display::Short::Table))
            return "table"_string;
        if (*this == Display::from_short(Display::Short::InlineTable))
            return "inline-table"_string;

        switch (m_value.outside_inside.outside) {
        case Outside::Block:
            builder.append("block"sv);
            break;
        case Outside::Inline:
            builder.append("inline"sv);
            break;
        case Outside::RunIn:
            builder.append("run-in"sv);
            break;
        }
        builder.append(' ');
        switch (m_value.outside_inside.inside) {
        case Inside::Flow:
            builder.append("flow"sv);
            break;
        case Inside::FlowRoot:
            builder.append("flow-root"sv);
            break;
        case Inside::Table:
            builder.append("table"sv);
            break;
        case Inside::Flex:
            builder.append("flex"sv);
            break;
        case Inside::Grid:
            builder.append("grid"sv);
            break;
        case Inside::Ruby:
            builder.append("ruby"sv);
            break;
        }
        if (m_value.outside_inside.list_item == ListItem::Yes)
            builder.append(" list-item"sv);
        break;
    case Type::Internal:
        switch (m_value.internal) {
        case Internal::TableRowGroup:
            builder.append("table-row-group"sv);
            break;
        case Internal::TableHeaderGroup:
            builder.append("table-header-group"sv);
            break;
        case Internal::TableFooterGroup:
            builder.append("table-footer-group"sv);
            break;
        case Internal::TableRow:
            builder.append("table-row"sv);
            break;
        case Internal::TableCell:
            builder.append("table-cell"sv);
            break;
        case Internal::TableColumnGroup:
            builder.append("table-column-group"sv);
            break;
        case Internal::TableColumn:
            builder.append("table-column"sv);
            break;
        case Internal::TableCaption:
            builder.append("table-caption"sv);
            break;
        case Internal::RubyBase:
            builder.append("ruby-base"sv);
            break;
        case Internal::RubyText:
            builder.append("ruby-text"sv);
            break;
        case Internal::RubyBaseContainer:
            builder.append("ruby-base-container"sv);
            break;
        case Internal::RubyTextContainer:
            builder.append("ruby-text-container"sv);
            break;
        }
        break;
    case Type::Box:
        switch (m_value.box) {
        case Box::Contents:
            builder.append("contents"sv);
            break;
        case Box::None:
            builder.append("none"sv);
            break;
        }
        break;
    };
    return MUST(builder.to_string());
}

}
