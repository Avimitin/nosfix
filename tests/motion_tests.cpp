#include "nosfix/motion.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace {

void CheckImpl(bool condition, const char* expression, int line) {
    if (!condition) {
        std::fprintf(stderr, "motion test failed at line %d: %s\n", line, expression);
        std::abort();
    }
}
#define Check(condition) CheckImpl((condition), #condition, __LINE__)

constexpr nosfix::GeometryKey kNoteA{0x1234, 2};

bool Near(float actual, double expected, double epsilon = 0.001) {
    return std::abs(static_cast<double>(actual) - expected) <= epsilon;
}

void TestEndpointsAndAlpha() {
    nosfix::GeometryPredictor predictor;
    std::array<nosfix::Vertex2, 2> vertices{{{10.0f, 20.0f}, {30.0f, 40.0f}}};
    Check(predictor.Capture(kNoteA, 100, vertices));
    Check(!predictor.HasMotionHistory());

    std::array<nosfix::Vertex2, 2> output{};
    Check(predictor.Evaluate(kNoteA, 100, 0.5, output));
    Check(output == vertices); // Missing history returns the exact current shape.

    vertices = {{{20.0f, 10.0f}, {40.0f, 30.0f}}};
    Check(predictor.Capture(kNoteA, 101, vertices));
    Check(predictor.HasMotionHistory());

    Check(predictor.Evaluate(kNoteA, 101, 0.0, output));
    Check(output == vertices); // Alpha zero is a direct copy, not arithmetic.
    Check(predictor.Evaluate(kNoteA, 101, 0.5, output));
    Check(Near(output[0].x, 25.0) && Near(output[0].y, 5.0));
    Check(Near(output[1].x, 45.0) && Near(output[1].y, 25.0));
    Check(predictor.Evaluate(kNoteA, 101, 1.0, output));
    Check(Near(output[0].x, 30.0) && Near(output[0].y, 0.0));
    Check(Near(output[1].x, 50.0) && Near(output[1].y, 20.0));

    Check(predictor.Evaluate(kNoteA, 101, -4.0, output));
    Check(output == vertices);
    Check(predictor.Evaluate(kNoteA, 101, 5.0, output));
    Check(Near(output[0].x, 30.0) && Near(output[0].y, 0.0)); // Clamped to one.
    Check(predictor.Evaluate(kNoteA, 101,
        std::numeric_limits<double>::quiet_NaN(), output));
    Check(output == vertices);
    Check(predictor.Evaluate(kNoteA, 101,
        std::numeric_limits<double>::infinity(), output));
    Check(output == vertices);

    std::array<nosfix::Vertex2, 1> wrong_size{};
    Check(!predictor.Evaluate(kNoteA, 101, 0.5, wrong_size));
    Check(!predictor.Evaluate({kNoteA.note_id + 1, kNoteA.primitive_role}, 101,
                              0.5, output));
}

void TestResetConditions() {
    nosfix::GeometryPredictor predictor;
    std::array<nosfix::Vertex2, 1> point{{{5.0f, 7.0f}}};
    std::array<nosfix::Vertex2, 1> output{};

    Check(predictor.Capture(kNoteA, 1, point));
    point[0].x = 9.0f;
    Check(predictor.Capture(kNoteA, 3, point)); // Missing tick 2.
    Check(!predictor.HasMotionHistory());
    Check(predictor.Evaluate(kNoteA, 3, 1.0, output));
    Check(output == point);

    const nosfix::GeometryKey other_role{kNoteA.note_id, kNoteA.primitive_role + 1};
    point[0].x = 12.0f;
    Check(predictor.Capture(other_role, 4, point));
    Check(!predictor.HasMotionHistory());
    Check(predictor.Evaluate(other_role, 4, 1.0, output));
    Check(output == point);

    point[0].x = 100.0f;
    Check(predictor.Capture(other_role, 5, point, true)); // Pause/seek/correction.
    Check(!predictor.HasMotionHistory());
    Check(predictor.Evaluate(other_role, 5, 1.0, output));
    Check(output == point);

    point[0].x = 101.0f;
    Check(predictor.Capture(other_role, 6, point));
    Check(predictor.HasMotionHistory());
    point[0].x = 102.0f;
    Check(predictor.Capture(other_role, 6, point)); // Same tick replaces current.
    Check(predictor.HasMotionHistory());
    Check(predictor.Evaluate(other_role, 6, 0.5, output));
    Check(Near(output[0].x, 103.0)); // Previous remains tick 5 (100), not tick 6 (101).
    Check(predictor.CurrentSnapshot(other_role, 6).size() == 1);
    Check(predictor.CurrentSnapshot(other_role, 5).empty());

    point[0].x = 1000.0f;
    Check(predictor.Capture(other_role, 6, point)); // Unsafe same-tick correction.
    Check(!predictor.HasMotionHistory());
    Check(predictor.Evaluate(other_role, 6, 0.5, output));
    Check(output == point);

    point[0].x = 1001.0f;
    Check(predictor.Capture(other_role, 7, point));
    std::array<nosfix::Vertex2, 2> changed_count{{{1, 2}, {3, 4}}};
    Check(predictor.Capture(other_role, 8, changed_count));
    Check(!predictor.HasMotionHistory());
    std::array<nosfix::Vertex2, 2> changed_output{};
    Check(predictor.Evaluate(other_role, 8, 0.5, changed_output));
    Check(changed_output == changed_count);
}

void TestUnsafeAndInvalidGeometry() {
    nosfix::GeometryPredictor predictor;
    std::array<nosfix::Vertex2, 1> point{{{0.0f, 0.0f}}};
    std::array<nosfix::Vertex2, 1> output{};
    Check(predictor.Capture(kNoteA, 1, point));

    point[0].x = 128.0f; // Exactly the accepted displacement threshold.
    Check(predictor.Capture(kNoteA, 2, point));
    Check(predictor.HasMotionHistory());
    Check(predictor.Evaluate(kNoteA, 2, 1.0, output));
    Check(Near(output[0].x, 256.0));

    point[0].x = 257.0f; // 129 px from the previous tick.
    Check(predictor.Capture(kNoteA, 3, point));
    Check(!predictor.HasMotionHistory());
    Check(predictor.Evaluate(kNoteA, 3, 1.0, output));
    Check(output == point);

    point[0].x = std::numeric_limits<float>::quiet_NaN();
    Check(!predictor.Capture(kNoteA, 4, point));
    Check(!predictor.HasCurrent());
    Check(!predictor.Evaluate(kNoteA, 4, 0.5, output));

    point[0] = {0.0f, 0.0f};
    Check(predictor.Capture(kNoteA, 5, point));
    point[0].x = static_cast<float>(nosfix::GeometryPredictor::kMaximumCoordinateMagnitude + 1.0);
    Check(!predictor.Capture(kNoteA, 6, point));
    Check(!predictor.HasCurrent());

    std::array<nosfix::Vertex2, 2> vertices{{{0, 0}, {0, 0}}};
    Check(predictor.Capture(kNoteA, 10, vertices));
    vertices[0].x = 1.0f; // Safe first vertex.
    vertices[1].x = 129.0f; // One unsafe vertex rejects all motion.
    Check(predictor.Capture(kNoteA, 11, vertices));
    Check(!predictor.HasMotionHistory());
    std::array<nosfix::Vertex2, 2> output_vertices{};
    Check(predictor.Evaluate(kNoteA, 11, 0.75, output_vertices));
    Check(output_vertices == vertices);
}

void TestAllDisplayRates() {
    constexpr std::array<unsigned, 7> rates{60, 90, 120, 144, 165, 240, 360};
    for (const auto hz : rates) {
        nosfix::GeometryPredictor predictor;
        std::uint64_t captured_tick = 0;
        std::array<nosfix::Vertex2, 1> current{};
        bool captured = false;
        const unsigned render_slots = 10 * hz;
        for (unsigned slot = 0; slot < render_slots; ++slot) {
            const double phase_tick = static_cast<double>(slot) * 60.0 / hz;
            const auto tick = static_cast<std::uint64_t>(std::floor(phase_tick));
            while (!captured || captured_tick < tick) {
                captured_tick = captured ? captured_tick + 1 : 0;
                current[0] = {static_cast<float>(captured_tick * 2),
                              static_cast<float>(captured_tick * -0.5)};
                Check(predictor.Capture(kNoteA, captured_tick, current));
                captured = true;
            }

            std::array<nosfix::Vertex2, 1> output{};
            const double alpha = phase_tick - tick;
            Check(predictor.Evaluate(kNoteA, tick, alpha, output));
            const double expected_x = tick >= 1 ? phase_tick * 2.0 : 0.0;
            const double expected_y = tick >= 1 ? phase_tick * -0.5 : 0.0;
            Check(Near(output[0].x, expected_x));
            Check(Near(output[0].y, expected_y));
        }
    }
}

void TestTenMinuteNoAccumulation() {
    nosfix::GeometryPredictor predictor;
    std::array<nosfix::Vertex2, 1> current{};
    std::array<nosfix::Vertex2, 1> output{};
    constexpr std::uint64_t ticks = 60 * 600;
    for (std::uint64_t tick = 0; tick <= ticks; ++tick) {
        current[0] = {static_cast<float>(tick * 0.25),
                      static_cast<float>(tick * -0.125)};
        Check(predictor.Capture(kNoteA, tick, current));
        Check(predictor.Evaluate(kNoteA, tick, 0.5, output));
        if (tick == 0) {
            Check(output == current);
        } else {
            Check(Near(output[0].x, static_cast<double>(tick) * 0.25 + 0.125));
            Check(Near(output[0].y, static_cast<double>(tick) * -0.125 - 0.0625));
        }
    }
}

} // namespace

int main() {
    TestEndpointsAndAlpha();
    TestResetConditions();
    TestUnsafeAndInvalidGeometry();
    TestAllDisplayRates();
    TestTenMinuteNoAccumulation();
}
