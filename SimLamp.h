// Copyright 2025-2026 Board of Regents of the University of Wisconsin System
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "SimHub.h"

#include "DeviceBase.h"

#include <atomic>
#include <string>
#include <utility>

class SimLamp : public CShutterBase<SimLamp> {
    std::string name_;
    SimHub *hub_ = nullptr;

    std::atomic<bool> isOpen_{false};
    std::atomic<double> intensity_{100.0};

  public:
    explicit SimLamp(std::string name) : name_(std::move(name)) {}

    ~SimLamp() { Shutdown(); }

    bool Busy() final { return false; }

    int Fire(double /*deltaT*/) final { return DEVICE_UNSUPPORTED_COMMAND; }

    int SetOpen(bool open = true) final {
        isOpen_ = open;
        return DEVICE_OK;
    }

    int GetOpen(bool &open) final {
        open = isOpen_;
        return DEVICE_OK;
    }

    void GetName(char *name) const final {
        CDeviceUtils::CopyLimitedString(name, name_.c_str());
    }

    int Initialize() final {
        hub_ = static_cast<SimHub *>(GetParentHub());
        if (!hub_) {
            return DEVICE_COMM_HUB_MISSING;
        }

        isOpen_ = false;

        int ret = CreateIntegerProperty(
            MM::g_Keyword_State, 0, false,
            new MM::ActionLambda(
                [this](MM::PropertyBase *pProp, MM::ActionType eAct) {
                    if (eAct == MM::BeforeGet) {
                        pProp->Set(isOpen_ ? 1L : 0L);
                    } else if (eAct == MM::AfterSet) {
                        long v{};
                        pProp->Get(v);
                        return SetOpen(v != 0);
                    }
                    return DEVICE_OK;
                }));
        if (ret != DEVICE_OK)
            return ret;
        ret = AddAllowedValue(MM::g_Keyword_State, "0");
        if (ret != DEVICE_OK)
            return ret;
        ret = AddAllowedValue(MM::g_Keyword_State, "1");
        if (ret != DEVICE_OK)
            return ret;

        ret = CreateFloatProperty(
            "Intensity", intensity_, false,
            new MM::ActionLambda(
                [this](MM::PropertyBase *pProp, MM::ActionType eAct) {
                    if (eAct == MM::BeforeGet) {
                        pProp->Set(intensity_.load());
                    } else if (eAct == MM::AfterSet) {
                        double v{};
                        pProp->Get(v);
                        intensity_ = v;
                    }
                    return DEVICE_OK;
                }));
        if (ret != DEVICE_OK)
            return ret;
        ret = SetPropertyLimits("Intensity", 0.0, 100.0);
        if (ret != DEVICE_OK)
            return ret;

        hub_->SetGetLampIntensityFunction([this] { return intensity_.load(); });
        return DEVICE_OK;
    }

    int Shutdown() final {
        if (hub_) {
            hub_->SetGetLampIntensityFunction([] { return 100.0; });
            hub_ = nullptr;
        }
        return DEVICE_OK;
    }
};
