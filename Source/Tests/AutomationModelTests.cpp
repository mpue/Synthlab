// JUCE-UnitTests fuer das Automation-Datenmodell.
// Aktivierung ueber Kommandozeile:  Synthlab runTests
// (siehe Source/Main.cpp).

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "../Automation/AutomationModel.h"

using namespace synthlab::automation;

class AutomationModelTests : public juce::UnitTest
{
public:
    AutomationModelTests() : juce::UnitTest ("automation/Model") {}

    void runTest() override
    {
        beginTest ("Lane: add point and read interpolated value");
        {
            Lane l (juce::Uuid(), "gain");
            l.addOrReplacePoint (0,    0.0f);
            l.addOrReplacePoint (1000, 1.0f);
            expectEquals (l.valueAt (0),    0.0f);
            expectEquals (l.valueAt (500),  0.5f);
            expectEquals (l.valueAt (1000), 1.0f);
            // Werte vor/nach den Stuetzpunkten klemmen.
            expectEquals (l.valueAt (-1),    0.0f);
            expectEquals (l.valueAt (5000),  1.0f);
        }

        beginTest ("Model: findOrCreate is idempotent");
        {
            Model m;
            auto& a = m.findOrCreate ("cutoff");
            const auto idA = a.getId();
            auto& b = m.findOrCreate ("cutoff");
            expect (a.getId() == b.getId(), "second findOrCreate must reuse existing lane");
            expect (idA == b.getId(),       "id must be preserved");
            expectEquals (m.numLanes(), 1);
        }

        beginTest ("Model: 50 lanes, remove one — others keep their ids");
        {
            Model m;
            std::vector<juce::Uuid> ids;
            for (int i = 0; i < 50; ++i)
                ids.push_back (m.findOrCreate ("p_" + juce::String (i)).getId());

            const auto removeId = ids[20];
            expect (m.removeLane (removeId));
            expectEquals (m.numLanes(), 49);

            for (int i = 0; i < 50; ++i)
            {
                if (i == 20) continue;
                auto* l = m.find ("p_" + juce::String (i));
                expect (l != nullptr, "lane " + juce::String (i) + " missing");
                if (l != nullptr) expect (l->getId() == ids[(size_t) i],
                                          "id of lane " + juce::String (i) + " changed");
            }
        }

        beginTest ("Lane: insert in different orders, sequence stays sorted");
        {
            Lane l (juce::Uuid(), "x");
            l.addOrReplacePoint (500,  0.5f);
            l.addOrReplacePoint (0,    0.0f);
            l.addOrReplacePoint (1000, 1.0f);
            l.addOrReplacePoint (250,  0.25f);

            const auto& pts = l.getPoints();
            expectEquals ((int) pts.size(), 4);
            for (size_t i = 1; i < pts.size(); ++i)
                expect (pts[i - 1].sampleTime < pts[i].sampleTime,
                        "points must be sorted by sampleTime");
        }

        beginTest ("Lane: replacing the same sampleTime does not duplicate");
        {
            Lane l (juce::Uuid(), "x");
            l.addOrReplacePoint (100, 0.1f);
            l.addOrReplacePoint (100, 0.9f);
            expectEquals ((int) l.getPoints().size(), 1);
            expectEquals (l.getPoints()[0].value, 0.9f);
        }

        beginTest ("Lane: step curve holds previous value until next point");
        {
            Lane l (juce::Uuid(), "x");
            l.addOrReplacePoint (0,    0.0f, CurveType::Step);
            l.addOrReplacePoint (100,  1.0f, CurveType::Linear);
            expectEquals (l.valueAt (50), 0.0f);
            expectEquals (l.valueAt (100), 1.0f);
        }

        beginTest ("Roundtrip: save -> load preserves values, ids and modes");
        {
            Model m;
            auto& la = m.findOrCreate ("gain");
            la.setMode (Mode::Write);
            la.setSampleAccurate (true);
            la.addOrReplacePoint (0,    0.1f);
            la.addOrReplacePoint (5000, 0.7f, CurveType::Step);

            auto& lb = m.findOrCreate ("pan");
            lb.addOrReplacePoint (1000, 0.5f);

            const auto idA = la.getId();
            const auto idB = lb.getId();

            const auto vt = m.toValueTree();

            Model m2;
            m2.fromValueTree (vt);
            expectEquals (m2.numLanes(), 2);

            auto* la2 = m2.find ("gain");
            auto* lb2 = m2.find ("pan");
            expect (la2 != nullptr && lb2 != nullptr);
            if (la2 == nullptr || lb2 == nullptr) return;

            expect (la2->getId() == idA, "lane id not preserved");
            expect (lb2->getId() == idB, "lane id not preserved");
            expect (la2->getMode() == Mode::Write);
            expect (la2->isSampleAccurate());
            expectEquals (la2->getPoints()[0].value, 0.1f);
            expectEquals (la2->getPoints()[1].value, 0.7f);
            expect (la2->getPoints()[1].curve == CurveType::Step);
        }

        beginTest ("Lane: empty -> valueAt returns default");
        {
            Lane l;
            expectEquals (l.valueAt (1234, 0.42f), 0.42f);
        }

        beginTest ("Roundtrip: 10 lanes x 100 points stays byte-identical");
        {
            Model m;
            std::vector<juce::Uuid> ids;
            for (int li = 0; li < 10; ++li)
            {
                auto& lane = m.findOrCreate ("p_" + juce::String (li));
                ids.push_back (lane.getId());
                lane.setMode ((li % 2 == 0) ? Mode::Write : Mode::Touch);
                for (int pi = 0; pi < 100; ++pi)
                    lane.addOrReplacePoint (juce::int64 (li * 1000 + pi * 17),
                                            (float) (pi % 7) / 6.0f,
                                            (pi % 3 == 0) ? CurveType::Step
                                                          : CurveType::Linear);
            }

            const auto vt = m.toValueTree();

            Model m2;
            m2.fromValueTree (vt);

            expectEquals (m2.numLanes(), 10);
            for (int li = 0; li < 10; ++li)
            {
                auto* a = m.find  ("p_" + juce::String (li));
                auto* b = m2.find ("p_" + juce::String (li));
                expect (a != nullptr && b != nullptr);
                if (a == nullptr || b == nullptr) continue;
                expect (a->getId() == b->getId(), "lane id changed for " + juce::String (li));
                expect (a->getMode() == b->getMode());
                expectEquals ((int) a->getPoints().size(), 100);
                expectEquals ((int) b->getPoints().size(), 100);
                for (size_t k = 0; k < 100; ++k)
                    expect (a->getPoints()[k] == b->getPoints()[k],
                            "point " + juce::String ((int) k) + " differs in lane "
                                + juce::String (li));
            }
        }
    }
};

static AutomationModelTests s_automationModelTests;
