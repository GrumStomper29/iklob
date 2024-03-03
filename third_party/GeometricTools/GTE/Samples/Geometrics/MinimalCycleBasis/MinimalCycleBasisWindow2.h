// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2023
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 6.0.2022.01.06

#pragma once

#include <Applications/Window2.h>
#include <Mathematics/ArbitraryPrecision.h>
#include <Mathematics/MinimalCycleBasis.h>
using namespace gte;

class MinimalCycleBasisWindow2 : public Window2
{
public:
    MinimalCycleBasisWindow2(Parameters& parameters);

    typedef BSNumber<UIntegerAP32> Rational;

    virtual void OnDisplay() override;
    virtual bool OnCharPress(uint8_t key, int32_t x, int32_t y) override;

private:
    bool SetEnvironment();
    void DrawTree(std::shared_ptr<MinimalCycleBasis<Rational>::Tree> const& tree);

    std::vector<std::array<Rational, 2>> mPositions;
    std::vector<std::array<int32_t, 2>> mEdges;
    std::vector<std::array<float, 2>> mFPositions;
    std::vector<std::array<int32_t, 2>> mSPositions;

    std::vector<std::shared_ptr<MinimalCycleBasis<Rational>::Tree>> mForest;
    bool mDrawRawData;
};
