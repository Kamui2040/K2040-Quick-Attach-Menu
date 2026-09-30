#include "AttachmentRuntimeModel.h"

#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::string JsonEscape(const std::string& value)
    {
        std::ostringstream out;

        for (char c : value) {
            switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                out << c;
                break;
            }
        }

        return out.str();
    }

    void WriteStringArray(std::ostringstream& json, const std::vector<std::string>& values)
    {
        json << "[";

        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                json << ",";
            }

            json << "\"" << JsonEscape(values[i]) << "\"";
        }

        json << "]";
    }
}

namespace k2040
{
    std::string BuildAttachmentRuntimeModelDebugJson()
    {
        std::ostringstream json;

        const std::vector<std::string> orderingSources = {
            "do_ModMenuSlotKeywordList is a global ordering / known-slot hint only.",
            "Fallout4.esm baseline contains the vanilla broad AP order.",
            "Dank_ECO.esp extends the same list with ECO/APF standardized AP keywords.",
            "Weapon-specific APs can exist and work even when absent from the global ordering list."
        };

        const std::vector<std::string> availabilitySources = {
            "WEAP APPR - Attach Parent Slots",
            "WEAP Object Template default OMODs",
            "Currently installed OMODs",
            "Provider OMOD Attach Parent Slots"
        };

        const std::vector<std::string> optionRules = {
            "An option is structurally valid when its consumed attach point is currently available.",
            "The OMOD target keyword must match the equipped weapon, normally through an ma_ keyword.",
            "Loose mod or inventory availability is a separate runtime filter, except for supported no-loose-mod toggles.",
            "Dependent child options are gated by provider OMODs that expose the child attach parent slot."
        };

        const std::vector<std::string> uspConfirmedPatterns = {
            "USP default/provider OMODs consume broad APs and expose USP-specific child APs.",
            "mod_USP_Mount_Off consumes ap_USP_Mount and exposes ap_gun_Scope.",
            "mod_USP_Mount_On consumes ap_USP_Mount and exposes ap_USP_Scope.",
            "Mounted sights consume ap_USP_Scope; non-mounted sights consume ap_gun_Scope.",
            "TLR-2 lower rail provider exposes ap_USP_ModRailLowerLSOptions for laser sub-options."
        };

        json
            << "{"
            << "\"globalSlotListRole\":\"orderingHintOnly\","
            << "\"requiresApInGlobalOrderList\":false,"
            << "\"actualAvailabilityModel\":\"WEAP_APPR_plus_default_OMODs_plus_installed_provider_OMODs\","
            << "\"categoryVisibilityRule\":\"show category when at least one menu option consumes an available attach point and passes target/inventory filters\","
            << "\"replacementSafetyRule\":\"when replacing or removing a provider OMOD, remove or replace dependent child OMODs first\","
            << "\"orderingSources\":";

        WriteStringArray(json, orderingSources);

        json << ",\"availabilitySources\":";
        WriteStringArray(json, availabilitySources);

        json << ",\"optionRules\":";
        WriteStringArray(json, optionRules);

        json << ",\"uspConfirmedPatterns\":";
        WriteStringArray(json, uspConfirmedPatterns);

        json << "}";

        return json.str();
    }
}
