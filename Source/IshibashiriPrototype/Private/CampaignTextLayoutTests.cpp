#if WITH_DEV_AUTOMATION_TESTS
#include "CampaignTextLayout.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignTextLayoutTest, "IshibashiriPrototype.Campaign.CardTextWrapping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCampaignTextLayoutTest::RunTest(const FString&)
{
    const auto Width = [](const FString& Value) { return static_cast<float>(Value.Len() * 10); };
    TestEqual(TEXT("Japanese wraps at display width"), CampaignTextLayout::Wrap(TEXT("白面の物語"), 30, 1, Width).Num(), 2);
    TestEqual(TEXT("Scale reduces available glyph count"), CampaignTextLayout::Wrap(TEXT("SHORT"), 30, 2, Width).Num(), 5);
    TestEqual(TEXT("Short English remains one line"), CampaignTextLayout::Wrap(TEXT("START"), 60, 1, Width).Num(), 1);
    TestTrue(TEXT("Empty text has no draw lines"), CampaignTextLayout::Wrap(TEXT(""), 1, 1, Width).IsEmpty());
    const TArray<FString> Narrow = CampaignTextLayout::Wrap(TEXT("ABC"), 0, 1, Width);
    TestEqual(TEXT("Narrow viewport remains finite"), Narrow.Num(), 3);
    return true;
}
#endif
