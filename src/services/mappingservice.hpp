// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/services.hpp>
#include <element/processor.hpp>
#include <element/controller.hpp>
#include <element/signals.hpp>

namespace element {

class Node;

class MappingService : public Service
{
public:
    MappingService();
    ~MappingService();

    void activate() override;
    void deactivate() override;
    void learn (const bool shouldLearn = true);
    bool isLearning() const;
    void remove (const ControllerMap&);

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> impl;
    SignalConnection capturedConnection;
    SignalConnection capturedParamConnection;
    void onControlCaptured();
    void onParameterCaptured (const Node&, int);
};

} // namespace element
