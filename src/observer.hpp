#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "event.hpp"

class Observer
{
public:
    virtual ~Observer() {}
	virtual void onNotify(Event event) = 0;
};