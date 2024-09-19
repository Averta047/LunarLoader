#pragma once

class CGuiPanel {
public:
    virtual ~CGuiPanel() = default;
    virtual void Render() = 0;
};