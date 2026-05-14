#ifndef TESTS_MOCKS_MOCKBUS_H
#define TESTS_MOCKS_MOCKBUS_H

#include "Interfaces/IBus.h"
#include <vector>
#include <string>

class MockBus : public IBus {
public:
    struct Published {
        Topic       topic;
        std::string payload;
    };

    std::vector<Published> published;

    void publish(Topic t, const char* payload) override {
        published.push_back({ t, payload ? payload : "" });
    }

    void clear() { published.clear(); }

    bool hasPublished(Topic t, const std::string& substr = "") const {
        for (const auto& p : published) {
            if (p.topic != t) continue;
            if (substr.empty() || p.payload.find(substr) != std::string::npos)
                return true;
        }
        return false;
    }

    size_t count(Topic t) const {
        size_t n = 0;
        for (const auto& p : published) if (p.topic == t) ++n;
        return n;
    }

    const Published* last(Topic t) const {
        for (int i = static_cast<int>(published.size()) - 1; i >= 0; --i)
            if (published[static_cast<size_t>(i)].topic == t)
                return &published[static_cast<size_t>(i)];
        return nullptr;
    }
};

#endif // TESTS_MOCKS_MOCKBUS_H
