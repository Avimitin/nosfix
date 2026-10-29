#include "nosfix/ui_motion.hpp"

#include <array>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>

namespace {

using Matrix = std::array<float, 16>;

Matrix Identity(float x = 0.0f, float y = 0.0f, float z = 0.0f) {
    Matrix value{};
    value[0] = value[5] = value[10] = value[15] = 1.0f;
    value[12] = x;
    value[13] = y;
    value[14] = z;
    return value;
}

void CheckNoPrediction(nosfix::UiTransformPredictor& predictor,
                       nosfix::UiNodeKey key, std::uint64_t tick,
                       const Matrix& latest) {
    Matrix output{};
    assert(predictor.Evaluate(key, tick, 0.75, output));
    assert(output == latest);
    assert(!predictor.HasHistory());
}

} // namespace

int main() {
    using nosfix::UiNodeKey;
    using nosfix::UiTransformPredictor;

    constexpr UiNodeKey key{0x1000, 0x12345678, 0x2000, 0x80};
    UiTransformPredictor predictor;
    Matrix first = Identity(10.0f, -4.0f, 2.0f);
    const Matrix original_first = first;
    assert(predictor.Capture(key, 100, first));
    assert(predictor.Capture(key, 100, first)); // Same sample is harmless.
    assert(!predictor.HasHistory());

    Matrix second = Identity(12.0f, -1.0f, 3.0f);
    second[0] = 0.8f; // Orientation/scale remains the latest exact value.
    const Matrix original_second = second;
    assert(predictor.Capture(key, 101, second));
    assert(predictor.HasHistory());
    assert(first == original_first && second == original_second);

    Matrix output{};
    assert(predictor.Evaluate(key, 101, 0.0, output));
    assert(output == second); // Alpha zero is the exact endpoint.
    assert(predictor.Evaluate(key, 101, 0.5, output));
    assert(output[12] == 13.0f && output[13] == 0.5f && output[14] == 3.5f);
    assert(output[0] == second[0]);
    assert(output[15] == 1.0f);
    assert(first == original_first && second == original_second);

    // A different matrix observed for the same node in one tick makes its
    // sample phase ambiguous and starts a fresh baseline.
    Matrix changed_same_tick = Identity(13.0f, -1.0f, 3.0f);
    changed_same_tick[0] = 0.75f;
    assert(predictor.Capture(key, 101, changed_same_tick));
    CheckNoPrediction(predictor, key, 101, changed_same_tick);

    Matrix third = Identity(15.0f, -1.0f, 3.0f);
    assert(predictor.Capture(key, 102, third));
    assert(predictor.HasHistory());

    const std::array<UiNodeKey, 4> changed_keys{
        UiNodeKey{key.node, key.allocation_stamp + 1, key.parent, key.type},
        UiNodeKey{key.node, key.allocation_stamp, key.parent + 1, key.type},
        UiNodeKey{key.node, key.allocation_stamp, key.parent, key.type + 1},
        UiNodeKey{key.node + 1, key.allocation_stamp, key.parent, key.type},
    };
    for (const auto& changed_key : changed_keys) {
        Matrix current = Identity(40.0f, 50.0f, 0.0f);
        assert(predictor.Capture(changed_key, 103, current));
        CheckNoPrediction(predictor, changed_key, 103, current);
        assert(predictor.Capture(changed_key, 104, Identity(42.0f, 52.0f, 0.0f)));
        assert(predictor.HasHistory());
    }

    Matrix after_gap = Identity(5.0f, 7.0f, 0.0f);
    assert(predictor.Capture(changed_keys.back(), 110, after_gap));
    CheckNoPrediction(predictor, changed_keys.back(), 110, after_gap);

    Matrix before_pause = Identity(7.0f, 8.0f, 0.0f);
    assert(predictor.Capture(changed_keys.back(), 111, before_pause));
    assert(predictor.HasHistory());
    Matrix after_pause = Identity(8.0f, 9.0f, 0.0f);
    assert(predictor.Capture(changed_keys.back(), 112, after_pause, true));
    CheckNoPrediction(predictor, changed_keys.back(), 112, after_pause);

    Matrix large_jump = Identity(140.0f, 9.0f, 0.0f);
    assert(predictor.Capture(changed_keys.back(), 113, large_jump));
    CheckNoPrediction(predictor, changed_keys.back(), 113, large_jump);

    Matrix invalid = Identity();
    invalid[12] = std::numeric_limits<float>::quiet_NaN();
    assert(!predictor.Capture(changed_keys.back(), 114, invalid));
    assert(!predictor.Evaluate(changed_keys.back(), 114, 0.5, output));

    Matrix projective = Identity();
    projective[3] = 0.00001f;
    assert(!predictor.Capture(changed_keys.back(), 115, projective));

    // Invalid phases are exact-current passthrough; finite phases clamp.
    assert(predictor.Capture(key, 200, Identity(1.0f, 2.0f, 0.0f)));
    assert(predictor.Capture(key, 201, Identity(3.0f, 4.0f, 0.0f)));
    const Matrix latest = Identity(3.0f, 4.0f, 0.0f);
    assert(predictor.Evaluate(key, 201,
        std::numeric_limits<double>::quiet_NaN(), output));
    assert(output == latest);
    assert(predictor.Evaluate(key, 201, 2.0, output));
    assert(output[12] == 5.0f && output[13] == 6.0f);

    // Stale tick/key requests do not produce a render copy.
    assert(!predictor.Evaluate(key, 200, 0.5, output));
    assert(!predictor.Evaluate(changed_keys[0], 201, 0.5, output));
}
