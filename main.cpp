#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

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
class Query;
class TagQuery {
public:
    const string tag;
    const std::shared_ptr<Query> next;
};
class Query {
    // This class should just be
    // using Query = std::variant<TagQuery, AttrQuery>;
    // but hackerrank doesn't support cpp-17 yet :(
public:
    Query(Query&& other)
    : kind{other.kind}
    , _attrQ{move(other._attrQ)}
    , _tagQ{move(other._tagQ)}
    { }
    Query(AttrQuery&& aq)
    : kind{Kind::Attr}
    , _attrQ{make_unique<AttrQuery>(aq)}
    , _tagQ{nullptr}
    { }

    Query(TagQuery&& tq)
    : kind{Kind::Tag}
    , _attrQ{nullptr}
    , _tagQ{make_unique<TagQuery>(tq)}
    { }

    enum class Kind {
        Tag, Attr
    };

    const Kind kind;

    const AttrQuery& getAttr() const {
        return *_attrQ;
    }
    const TagQuery& getTag() const {
        return *_tagQ;
    }

    template<typename R>
    R visit(
        std::function<R(const AttrQuery&)> aFunc,
        std::function<R(const TagQuery&)> tFunc
    ) const {
        if (kind == Kind::Attr) return aFunc(*_attrQ);
        return tFunc(*_tagQ);
    }
private:
    unique_ptr<AttrQuery> _attrQ;
    unique_ptr<TagQuery> _tagQ;
};

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

string executeQuery(const Element& el, const Query& q) {
    return q.visit<string>(
        [&el](const AttrQuery& aq) {
            auto pos = el.attrs.find(aq.attr);
            if (pos == end(el.attrs)) return ""s;
            return pos->second;
        },
        [&el](const TagQuery& tq) {
            auto pos = find_if(begin(el.children), end(el.children), [&](Element child){ return child.tag == tq.tag; });
            if (pos == end(el.children)) return ""s;
            return executeQuery(*pos, *tq.next);
        });
}

string executeQuery(const Tree& t, const Query& q) {
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


TEST_CASE("parseTree") {
    stringstream empty{"<empty></empty>after"};
    auto emptyElem = parseTree(empty);
    REQUIRE(emptyElem.root.tag == "empty");
    REQUIRE(emptyElem.root.attrs.size() == 0);
    REQUIRE(emptyElem.root.attrs.size() == 0);
    string next;
    empty >> next;
    REQUIRE(next == "after");

    stringstream attrsOnly{"<attrs-only one = \"1\" two = \"2\"></attrs-only>"};
    auto attrsElem = parseTree(attrsOnly);
    REQUIRE(attrsElem.root.tag == "attrs-only");
    REQUIRE(attrsElem.root.attrs.at("one") == "1");
    REQUIRE(attrsElem.root.attrs.at("two") == "2");
    REQUIRE(attrsElem.root.attrs.size() == 2);
    REQUIRE(attrsElem.root.children.size() == 0);
}

TEST_CASE("parseQuery") {
    stringstream simple{"tag1~value"};
    auto simpleQ = parseQuery(simple);
    REQUIRE(simpleQ.kind == Query::Kind::Tag);
    REQUIRE(simpleQ.getTag().tag == "tag1");
    const Query& inner = *simpleQ.getTag().next;
    REQUIRE(inner.kind == Query::Kind::Attr);
    REQUIRE(inner.getAttr().attr == "value");
}

TEST_CASE("executeQuery") {

    Element link{ "a", unordered_map<string, string>{{"href", "google.com"}}, {} };
    Tree root {link};
    Query query = TagQuery{"a", make_shared<Query>(AttrQuery{"href"})};
    auto res = executeQuery(root, query);
    REQUIRE(res != "");
    REQUIRE(res == "google.com");
}
#else
int main() {
    int numTagLines, numQueries;
    cin >> numTagLines >> numQueries;
    Tree tree = parseTree(cin);
    string line;
    for (; numQueries--;) {
        auto q = parseQuery(cin);
        auto res = executeQuery(tree, q);
        cout << (res == "" ? "Not Found!" : res) << endl;
    }
    return 0;
}
#endif

