#pragma once
#include <Arduino.h>
#include "functions.h"

class ClickHandler
{
public:
    using Callback = void (*)();

    ClickHandler(int pin, bool activeLow = true);

    void begin();
    void update();

    void onSingleClick(Callback cb);
    void onDoubleClick(Callback cb);
    void onTripleClick(Callback cb);
    void onQuadrupleClick(Callback cb);

private:
    int _pin;
    bool _activeLow;

    unsigned long _debounceMs = 50;
    unsigned long _multiClickTimeout = 400;

    bool _lastState;
    bool _buttonPressed = false;
    int _clickCount = 0;
    unsigned long _lastDebounceTime = 0;
    unsigned long _lastClickTime = 0;

    Callback _singleClickCb = nullptr;
    Callback _doubleClickCb = nullptr;
    Callback _tripleClickCb = nullptr;
    Callback _quadrupleClick = nullptr;

    void trigger(int count);
};
