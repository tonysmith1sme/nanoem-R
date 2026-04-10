/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#include "emapp/BezierCurve.h"

#include "emapp/private/CommonInclude.h"

namespace nanoem {

const Vector2 BezierCurve::kP0 = Vector2(0);
const Vector2 BezierCurve::kP1 = Vector2(127);

BezierCurve::BezierCurve(const Vector2U8 &c0, const Vector2U8 &c1, nanoem_frame_index_t interval)
    : m_c0(c0)
    , m_c1(c1)
    , m_interval(interval)
{
}

BezierCurve::~BezierCurve() NANOEM_DECL_NOEXCEPT
{
}

nanoem_f32_t
BezierCurve::value(nanoem_f32_t value) const NANOEM_DECL_NOEXCEPT
{
    const nanoem_f32_t x = glm::clamp(value, 0.0f, 1.0f);
    const nanoem_f32_t x1 = m_c0.x / 127.0f;
    const nanoem_f32_t x2 = m_c1.x / 127.0f;
    const nanoem_f32_t y1 = m_c0.y / 127.0f;
    const nanoem_f32_t y2 = m_c1.y / 127.0f;
    const nanoem_f32_t t = solveT(x, x1, x2);
    return evaluate(0.0f, y1, y2, 1.0f, t);
}

nanoem_frame_index_t
BezierCurve::length() const NANOEM_DECL_NOEXCEPT
{
    return m_interval;
}

BezierCurve::Pair
BezierCurve::split(const nanoem_f32_t t) const
{
    const nanoem_f32_t tv = glm::clamp(t, 0.0f, 1.0f);
    PointList points(4), left, right;
    points[0] = kP0;
    points[1] = m_c0;
    points[2] = m_c1;
    points[3] = kP1;
    splitBezierCurve(points, tv, left, right);
    BezierCurve *lvalue = nanoem_new(BezierCurve(left[1], left[2], nanoem_frame_index_t(m_interval * tv)));
    BezierCurve *rvalue = nanoem_new(BezierCurve(right[1], right[2], nanoem_frame_index_t(m_interval * (1.0f - tv))));
    return Pair(lvalue, rvalue);
}

Vector4U8
BezierCurve::toParameters() const NANOEM_DECL_NOEXCEPT
{
    return Vector4U8(m_c0, m_c1);
}

Vector2U8
BezierCurve::c0() const NANOEM_DECL_NOEXCEPT
{
    return m_c0;
}

Vector2U8
BezierCurve::c1() const NANOEM_DECL_NOEXCEPT
{
    return m_c1;
}

nanoem_u64_t
BezierCurve::toHash(const nanoem_u8_t *parameters, nanoem_frame_index_t interval) NANOEM_DECL_NOEXCEPT
{
    Hash hash;
    hash.value = 0;
    hash.c0x = parameters[0];
    hash.c0y = parameters[1];
    hash.c1x = parameters[2];
    hash.c1y = parameters[3];
    hash.interval = interval;
    return hash.value;
}

nanoem_f32_t
BezierCurve::evaluate(
    const nanoem_f32_t p0, const nanoem_f32_t p1, const nanoem_f32_t p2, const nanoem_f32_t p3, const nanoem_f32_t t)
    NANOEM_DECL_NOEXCEPT
{
    const nanoem_f32_t it = 1.0f - t;
    return (it * it * it * p0) + (3.0f * it * it * t * p1) + (3.0f * it * t * t * p2) + (t * t * t * p3);
}

nanoem_f32_t
BezierCurve::derivative(
    const nanoem_f32_t p0, const nanoem_f32_t p1, const nanoem_f32_t p2, const nanoem_f32_t p3, const nanoem_f32_t t)
    NANOEM_DECL_NOEXCEPT
{
    const nanoem_f32_t it = 1.0f - t;
    return (3.0f * it * it * (p1 - p0)) + (6.0f * it * t * (p2 - p1)) + (3.0f * t * t * (p3 - p2));
}

nanoem_f32_t
BezierCurve::solveT(const nanoem_f32_t x, const nanoem_f32_t x1, const nanoem_f32_t x2) NANOEM_DECL_NOEXCEPT
{
    nanoem_f32_t t = x;
    for (int i = 0; i < 8; i++) {
        const nanoem_f32_t current = evaluate(0.0f, x1, x2, 1.0f, t) - x;
        if (glm::abs(current) < 1.0e-6f) {
            return glm::clamp(t, 0.0f, 1.0f);
        }
        const nanoem_f32_t slope = derivative(0.0f, x1, x2, 1.0f, t);
        if (glm::abs(slope) < 1.0e-6f) {
            break;
        }
        t -= current / slope;
    }
    nanoem_f32_t left = 0.0f;
    nanoem_f32_t right = 1.0f;
    t = x;
    for (int i = 0; i < 24; i++) {
        const nanoem_f32_t current = evaluate(0.0f, x1, x2, 1.0f, t);
        if (glm::abs(current - x) < 1.0e-6f) {
            break;
        }
        if (current < x) {
            left = t;
        }
        else {
            right = t;
        }
        t = (left + right) * 0.5f;
    }
    return glm::clamp(t, 0.0f, 1.0f);
}

void
BezierCurve::splitBezierCurve(const PointList &points, nanoem_f32_t t, PointList &left, PointList &right)
{
    if (points.size() == 1) {
        const Vector2 point(points[0]);
        left.push_back(point);
        right.push_back(point);
    }
    else {
        const nanoem_rsize_t length = points.size() - 1;
        PointList newPoints(length);
        left.push_back(points[0]);
        right.push_back(points[length]);
        for (nanoem_rsize_t i = 0; i < length; i++) {
            newPoints[i] = (1.0f - t) * points[i] + t * points[i + 1];
        }
        splitBezierCurve(newPoints, t, left, right);
    }
}

} /* namespace nanoem */
