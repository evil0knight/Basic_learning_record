#include "user_init.h"

#include "user_periph_setup.h"
#include "user_task_reso_config.h"

static bool g_user_app_initialized;

osal_status_t user_app_init(void)
{
    osal_status_t status;

    if (g_user_app_initialized)
    {
        return OSAL_SUCCESS;
    }

    app_periph_register();

    status = user_os_resources_init();
    if (status != OSAL_SUCCESS)
    {
        return status;
    }

    status = user_tasks_start();
    if (status != OSAL_SUCCESS)
    {
        user_os_resources_deinit();
        return status;
    }

    g_user_app_initialized = true;
    return OSAL_SUCCESS;
}

void user_app_deinit(void)
{
    user_os_resources_deinit();
    g_user_app_initialized = false;
}
