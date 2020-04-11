#include <cmath>
#include <cstdio>
#include <vector>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <regex>
#include <deque>
#include <cassert>
#include <variant>
#include <memory>

using namespace std;

class Element {
public:
    const string tag;
    const unordered_map<string, string> attrs;
    const vector<Element> children;
};

class Tree {
public:
    Element root;
};


class AttrQuery {
public:
    const string attr;
};
class TagQuery {
public:
    const string tag;
    const std::shared_ptr<std::variant<TagQuery, AttrQuery>> next;
};
using Query = std::variant<TagQuery, AttrQuery>;

void readChar(istream& stream, char expected) {
    char actual;
    stream.get(actual);
    assert(actual == expected);
}

string readUntil(istream& stream, const char* anyOf) {
    string ret;
    while (strchr(anyOf, stream.peek()) == nullptr) {
        ret.push_back(stream.get());
    }
    return ret;
}

string readWhile(istream& stream, const char* anyOf) {
    string ret;
    while (strchr(anyOf, stream.peek()) != nullptr) {
        ret.push_back(stream.get());
    }
    return ret;
}


Query parseQuery(istream& stream) {
    (void) readWhile(stream, "\r\n ");
    string tag = readUntil(stream, ".~");
    char sep = stream.get();
    if (sep == '.') {
        return TagQuery{tag, make_shared<Query>(parseQuery(stream))};
    }
    string attr;
    stream >> attr;
    return TagQuery{tag, make_shared<Query>(AttrQuery{attr})};
}

optional<string> executeQuery(Element el, Query q) {
    if (std::holds_alternative<TagQuery>(q)) {
        auto tq = std::get<TagQuery>(q);
        auto pos = find_if(begin(el.children), end(el.children), [&](Element child){ return child.tag == tq.tag; });
        if (pos == end(el.children)) return std::nullopt;
        return executeQuery(*pos, *tq.next);
    } else {
        auto aq = std::get<AttrQuery>(q);
        auto pos = el.attrs.find(aq.attr);
        if (pos == end(el.attrs)) return std::nullopt;
        return pos->second;
    }
}

optional<string> executeQuery(Tree t, Query q) {
    Element root{"root", {}, {t.root}};
    return executeQuery(root, q);
}

unordered_map<string, string> parseAttrs(istream& stream) {
    unordered_map<string, string> attrs;
    string key, value;
    while (stream.peek() != '>') {
        stream >> key;
        readChar(stream, ' ');
        readChar(stream, '=');
        readChar(stream, ' ');
        readChar(stream, '"');
        value = readUntil(stream, "\"");
        readChar(stream, '"');
        attrs[key] = value;
    }
    return attrs;
}

Element parseElemExceptLangle(istream&);
vector<Element> parseChildrenAndEndTag(istream& stream, string tag) {
    vector<Element> children;
    (void) readUntil(stream, "<");
    readChar(stream, '<');
    while (stream.peek() != '/') {
        children.push_back(parseElemExceptLangle(stream));
        (void) readUntil(stream, "<");
        readChar(stream, '<');
    }
    // read the end tag
    readChar(stream, '/');
    string endTag{readUntil(stream, ">")};
    readChar(stream, '>');
    return children;
}

Element parseElemExceptLangle(istream& stream) {
    string tag = readUntil(stream, " >");
    auto attrs = parseAttrs(stream);
    readChar(stream, '>');
    auto children = parseChildrenAndEndTag(stream, tag);
    return Element{tag, attrs, children};
}


Tree parseTree(istream& stream) {
    (void) readUntil(stream, "<");
    readChar(stream, '<');
    return Tree{parseElemExceptLangle(stream)};
}



#ifdef TEST
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("readUntil") {
    stringstream stream{"\"value-pattern\">more"};
    string value{readUntil(stream, "> \n")};
    REQUIRE(value == "\"value-pattern\"");
    REQUIRE(stream.peek() == '>');
}

TEST_CASE("parseAttrs") {
    stringstream stream{" value = \"HelloWorld\">more"};

    unordered_map<string, string> attrs {parseAttrs(stream)};
    string rest;
    stream >> rest;
    REQUIRE(rest == ">more");
    REQUIRE(attrs.at("value") == "HelloWorld");

    stringstream multiple{" one = \"1\" two = \"2\">done"};
    attrs = parseAttrs(multiple);
    multiple >> rest;
    REQUIRE(attrs.at("one") == "1");
    REQUIRE(attrs.at("two") == "2");
    REQUIRE(attrs.size() == 2);
    REQUIRE(rest == ">done");
}

TEST_CASE("parseChildrenAndEndTag") {
    stringstream stream{"\n</end><another-tag></another-tag>"};
    auto children = parseChildrenAndEndTag(stream, "end");
    REQUIRE(children.size() == 0);
    REQUIRE(stream.peek() == '<');
}


TEST_CASE("parseElem") {
    stringstream empty{"<empty></empty>after"};
    auto emptyElem = parseElem(empty);
    REQUIRE(emptyElem.tag == "empty");
    REQUIRE(emptyElem.attrs.size() == 0);
    REQUIRE(emptyElem.attrs.size() == 0);
    string next;
    empty >> next;
    REQUIRE(next == "after");

    stringstream attrsOnly{"<attrs-only one = \"1\" two = \"2\"></attrs-only>"};
    auto attrsElem = parseElem(attrsOnly);
    REQUIRE(attrsElem.tag == "attrs-only");
    REQUIRE(attrsElem.attrs.at("one") == "1");
    REQUIRE(attrsElem.attrs.at("two") == "2");
    REQUIRE(attrsElem.attrs.size() == 2);
    REQUIRE(attrsElem.children.size() == 0);
}

TEST_CASE("parseQuery") {
    stringstream simple{"tag1~value"};
    auto simpleQ = parseQuery(simple);
    REQUIRE(holds_alternative<TagQuery>(simpleQ));
    REQUIRE(get<TagQuery>(simpleQ).tag == "tag1");
    auto inner = *get<TagQuery>(simpleQ).next;
    REQUIRE(holds_alternative<AttrQuery>(inner));
    REQUIRE(get<AttrQuery>(inner).attr == "value");
}

TEST_CASE("executeQuery") {

    Element link{ "a", unordered_map<string, string>{{"href", "google.com"}}, {} };
    Element root {"root", {}, {link}};
    Query query = TagQuery{"a", make_shared<Query>(AttrQuery{"href"})};
    auto res = executeQuery(root, query);
    REQUIRE(!!res);
    REQUIRE(*res == "google.com");
}
#else
int main() {
    int numTagLines, numQueries;
    cin >> numTagLines >> numQueries;
    Tree tree = parseTree(cin);
    string line;
    for (; numQueries--; numQueries) {
        auto q = parseQuery(cin);
        auto res = executeQuery(tree, q);
        cout << res.value_or("Not Found!") << endl;
    }
    return 0;
}
#endif

