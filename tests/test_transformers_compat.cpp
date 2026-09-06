// Regression tests for the Transformers-compatibility changes on this fork.
// Built with UJSON_USE_ORDERED_JSON=1 and JINJA_PRESERVE_JSON_OBJECT_ORDER=1.
#include <cassert>
#include <iostream>
#include <string>

#include "jinja.hpp"

namespace {

int failures = 0;

void expect(const std::string& name, const std::string& got, const std::string& want) {
    if (got == want) return;
    std::cerr << "FAIL " << name << "\n  want: " << want << "\n  got:  " << got << "\n";
    ++failures;
}

std::string render(const std::string& source, const jinja::json& context) {
    jinja::Template tpl(source);
    return tpl.render(context);
}

jinja::json messagesContext() {
    jinja::json messages = jinja::json::array();
    messages.push_back({{"role", "user"}, {"content", "hi"}});
    messages.push_back({{"role", "assistant"}, {"content", "yo"}});
    messages.push_back({{"role", "tool"}, {"content", "42"}});
    jinja::json context = jinja::json::object();
    context["messages"] = messages;
    return context;
}

}  // namespace

int main() {
    // Object construction from an initializer list stores the value, not the
    // whole [key, value] pair.
    {
        jinja::json context = jinja::json::object();
        context["m"] = jinja::json({{"role", "user"}, {"content", "hi"}});
        expect("initializer-list object stores values", render("{{ m.role }}/{{ m.content }}", context), "user/hi");
    }

    // Insertion order is preserved for tojson and for object iteration.
    {
        jinja::json value = jinja::json::object();
        value["zeta"] = 1;
        value["alpha"] = 2;
        jinja::json context = jinja::json::object();
        context["value"] = value;
        expect("tojson keeps insertion order", render("{{ value | tojson }}", context), "{\"zeta\": 1, \"alpha\": 2}");
        expect("items keeps insertion order", render("{% for k, v in value.items() %}{{ k }}={{ v }};{% endfor %}", context),
               "zeta=1;alpha=2;");
        expect("keys/values/get", render("{{ value.keys()|length }}|{{ value.values()|length }}|{{ value.get('alpha') }}|{{ value.get('nope', 'dflt') }}", context),
               "2|2|2|dflt");
    }

    // A value read from the scope stays valid after a later insertion.
    expect("set of a scope-owned value does not dangle",
           render("{% for m in messages %}{% set c = m.role %}[{{ c }}]{% endfor %}", messagesContext()),
           "[user][assistant][tool]");

    // loop neighbours and reverse indices.
    expect("loop.previtem and loop.nextitem",
           render("{% for m in messages %}{% if loop.previtem is defined %}{{ loop.previtem.role }}{% else %}none{% endif %}>"
                  "{{ m.role }}>{% if loop.nextitem is defined %}{{ loop.nextitem.role }}{% else %}none{% endif %};{% endfor %}",
                  messagesContext()),
           "none>user>assistant;user>assistant>tool;assistant>tool>none;");
    expect("loop.revindex", render("{% for m in messages %}{{ loop.revindex }}{{ loop.revindex0 }},{% endfor %}", messagesContext()),
           "32,21,10,");

    // Block assignment.
    expect("block set captures the rendered body",
           render("{% set block %}A{{ messages|length }}{% for m in messages %}:{{ m.role }}{% endfor %}{% endset %}[{{ block }}]{{ block|length }}",
                  messagesContext()),
           "[A3:user:assistant:tool]22");

    // min and max filters.
    expect("min and max", render("{{ [3, 1, 2]|min }}|{{ [3, 1, 2]|max }}|{{ ['b', 'a']|min }}", jinja::json::object()), "1|3|a");

    // Python str() printing for non-string values.
    {
        jinja::json context = jinja::json::object();
        context["obj"] = jinja::json::object();
        context["obj"]["a"] = 1;
        expect("print uses Python str semantics",
               render("{{ none }}|{{ true }}|{{ false }}|{{ obj }}|{{ [1, 2] }}", context), "None|True|False|{'a': 1}|[1, 2]");
    }

    // List concatenation and namespace write-back from an inner scope.
    expect("list concat and namespace update across scopes",
           render("{% set ns = namespace(n=0, seen=[]) %}{% for m in messages %}{% set ns.n = ns.n + 1 %}"
                  "{% set ns.seen = ns.seen + [m.role] %}{% endfor %}{{ ns.n }}:{{ ns.seen|length }}:{{ ns.seen[2] }}",
                  messagesContext()),
           "3:3:tool");

    if (failures == 0) std::cout << "transformers compat tests passed\n";
    return failures == 0 ? 0 : 1;
}
