#include "mercury/ServoV1.h"
#include "mercury/Mercury.h"
#include "mercury/MercuryTools.h"
#include "mercury/ControlTablesMercury.h"
#include "ControlTables.h"

ServoV1::ServoV1(int mercury_id, int mercury_model, int control_mode):
    ServoMercury(V1_control_table, mercury_id, mercury_model, control_mode)
{
    runningDegrees = 300;
    steps = 1024;
}

ServoV1::~ServoV1()
{
}

/* ************************************************************************** */

int ServoV1::getCwComplianceMargin()
{
    std::lock_guard <std::mutex> lock(access);
    return registerTableValues[gid(REG_CW_COMPLIANCE_MARGIN)];
}

int ServoV1::getCcwComplianceMargin()
{
    std::lock_guard <std::mutex> lock(access);
    return registerTableValues[gid(REG_CCW_COMPLIANCE_MARGIN)];
}

int ServoV1::getCwComplianceSlope()
{
    std::lock_guard <std::mutex> lock(access);
    return registerTableValues[gid(REG_CW_COMPLIANCE_SLOPE)];
}

int ServoV1::getCcwComplianceSlope()
{
    std::lock_guard <std::mutex> lock(access);
    return registerTableValues[gid(REG_CCW_COMPLIANCE_SLOPE)];
}
