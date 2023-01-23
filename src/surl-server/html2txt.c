/* 
 * https://github.com/spartrekus/html2txt-gumbo
 *
 * Copyright (C) https://github.com/spartrekus
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <gumbo.h>

static char *buffer_printf(char *buffer, const char *format, ...) {
  int w = 0;
  va_list args;
  
  va_start(args, format);
  w = vsprintf(buffer, format, args);
  va_end(args);
  
  return buffer + w;
}

static char *print_norm(char *buffer, const char *text)
{
    char *str, *token, *seps, *spaces;
    int length, trailing;

    seps = " \t\n\r";
    length = strlen(text);
    str = (char *) malloc(length + 1);
    strcpy(str, text);
    spaces = str;
    while (isspace(*spaces)) {
        if (*spaces == ' ')
            buffer = buffer_printf(buffer, " ");
        spaces++;
    }
    trailing = 0;
    spaces = str + length - 1;
    while (isspace(*spaces)) {
        if (*spaces == ' ')
            trailing++;
        spaces--;
    }
    token = strtok(str, seps);
    buffer = buffer_printf(buffer, "%s", token);
    token = strtok(NULL, seps);
    while (token) {
        buffer = buffer_printf(buffer, " %s", token);
        token = strtok(NULL, seps);
    }
    while (trailing--)
        buffer = buffer_printf(buffer, " ");
    free(str);

    return buffer;
}

static char *dump_tree(GumboNode *node, char *buffer, int plain)
{
    GumboVector *children;
    GumboAttribute *href, *src, *alt;
    int i;

    if (node->type == GUMBO_NODE_TEXT) {
        if (plain)
            buffer = buffer_printf(buffer, "%s", node->v.text.text);
        else
            buffer = print_norm(buffer, node->v.text.text);
    } else if (
        node->type == GUMBO_NODE_ELEMENT &&
        node->v.element.tag != GUMBO_TAG_SCRIPT &&
        node->v.element.tag != GUMBO_TAG_STYLE
    ) {
        if (node->v.element.tag == GUMBO_TAG_BR) {
            buffer = buffer_printf(buffer, "\n");
            return buffer;
        }
        plain = (
            node->v.element.tag == GUMBO_TAG_CODE ||
            node->v.element.tag == GUMBO_TAG_PRE
        );
        if (node->v.element.tag == GUMBO_TAG_LI)
            buffer = buffer_printf(buffer, "* ");
        if (
            node->v.element.tag == GUMBO_TAG_H1 ||
            node->v.element.tag == GUMBO_TAG_H2 ||
            node->v.element.tag == GUMBO_TAG_H3 ||
            node->v.element.tag == GUMBO_TAG_H4 ||
            node->v.element.tag == GUMBO_TAG_H5 ||
            node->v.element.tag == GUMBO_TAG_H6
        )
            buffer = buffer_printf(buffer, "\n\n");
        children = &node->v.element.children;
        for (i = 0; i < (int) children->length; i++)
            buffer = dump_tree((GumboNode *) children->data[i], buffer, plain);
        if (
            node->v.element.tag == GUMBO_TAG_TITLE ||
            node->v.element.tag == GUMBO_TAG_H1 ||
            node->v.element.tag == GUMBO_TAG_H2 ||
            node->v.element.tag == GUMBO_TAG_H3 ||
            node->v.element.tag == GUMBO_TAG_H4 ||
            node->v.element.tag == GUMBO_TAG_H5 ||
            node->v.element.tag == GUMBO_TAG_H6 ||
            node->v.element.tag == GUMBO_TAG_P
        )
            buffer = buffer_printf(buffer, "\n\n");
        else if (
            node->v.element.tag == GUMBO_TAG_LI ||
            node->v.element.tag == GUMBO_TAG_TR
        )
            buffer = buffer_printf(buffer, "\n");
        else if (node->v.element.tag == GUMBO_TAG_TD)
            buffer = buffer_printf(buffer, "\t");
        else if (node->v.element.tag == GUMBO_TAG_A) {
            href = gumbo_get_attribute(&node->v.element.attributes, "href");
            if (href)
                buffer = buffer_printf(buffer, " <%s>", href->value);
        }
        else if (node->v.element.tag == GUMBO_TAG_IMG) {
            src = gumbo_get_attribute(&node->v.element.attributes, "src");
            alt = gumbo_get_attribute(&node->v.element.attributes, "alt");
            if (alt && strlen(alt->value))
                buffer = buffer_printf(buffer, "\n(image: %s <%s>)\n", alt->value, src->value);
            else
                buffer = buffer_printf(buffer, "\n(image: <%s>)\n", src->value);
        }
    }
    return buffer;
}

char *html2text(char *html) {
    char *text;
    size_t buf_size;
    GumboOutput *parsed_html = gumbo_parse(html);

    buf_size = strlen(html) * 2;
    text = malloc(buf_size);
    memset(text, 0, buf_size);
    dump_tree(parsed_html->root, text, 0);
  
    gumbo_destroy_output(&kGumboDefaultOptions, parsed_html);
    return text;
}
