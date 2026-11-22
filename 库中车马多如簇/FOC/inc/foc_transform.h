#ifndef FOC_TRANSFORM_H
#define FOC_TRANSFORM_H
#include <stdint.h>
typedef enum
{
    CLARKE_OK       = 0,    /* 计算成功   */
    CLARKE_ERR_NULL = 1     /* 空指针     */
} clarke_status_t;

typedef enum
{
    PARK_OK       = 0,  /* 计算成功       */
    PARK_ERR_NULL = 1,  /* 空指针         */
    PARK_ERR_LUT  = 2   /* LUT查表失败    */
} park_status_t;

typedef enum
{
    INV_PARK_OK            = 0, /* 计算成功           */
    INV_PARK_ERR_NULL      = 1, /* 空指针             */
    INV_PARK_ERR_LUT       = 2  /* LUT查表失败        */
} inv_park_status_t;

typedef enum
{
    SVPWM_OK            = 0,    /* 计算成功           */
    SVPWM_ERR_NULL      = 1,    /* 空指针             */
    SVPWM_ERR_PARAM     = 2     /* 母线倒数或周期非法 */
} svpwm_status_t;

typedef struct
{
    float    u_alpha;   /* α轴目标电压 (V)              */
    float    u_beta;    /* β轴目标电压 (V)              */
    float    inv_v_dc;  /* 初始化计算的母线倒数 (1/V)，必须>0。 */
    float    t_pwm;     /* PWM周期 (s), 必须>0          */
    float    k0;        /* 零矢量系数 [0,1], 0.5=7段式  */
    uint16_t arr;       /* 定时器ARR值                   */
} svpwm_input_t;

typedef struct
{
    uint16_t ccr_u;     /* U相CCR, 直接写TIMx->CCR1     */
    uint16_t ccr_v;     /* V相CCR, 直接写TIMx->CCR2     */
    uint16_t ccr_w;     /* W相CCR, 直接写TIMx->CCR3     */
    uint8_t  sector;    /* 本次所在扇区1~6, 调试用       */
} svpwm_output_t;

clarke_status_t clarke_transform(
    const float          i_a,
    const float          i_b,
    float    * const p_i_alpha,
    float    * const  p_i_beta );

clarke_status_t clarke_transform_abc(
    const float          i_a,
    const float          i_b,
    const float          i_c,
    float    * const p_i_alpha,
    float    * const  p_i_beta );

park_status_t park_transform(
    const float      i_alpha,
    const float       i_beta,
    const float      theta_e,
    float    * const    p_i_d,
    float    * const    p_i_q );

park_status_t park_transform_sc(
    const float      i_alpha,
    const float       i_beta,
    const float    sin_theta,
    const float    cos_theta,
    float    * const    p_i_d,
    float    * const    p_i_q );

inv_park_status_t inverse_park(
    const float          v_d,
    const float          v_q,
    const float      theta_e,
    float    * const p_v_alpha,
    float    * const  p_v_beta );

inv_park_status_t inverse_park_sc(
    const float          v_d,
    const float          v_q,
    const float    sin_theta,
    const float    cos_theta,
    float    * const p_v_alpha,
    float    * const  p_v_beta );

svpwm_status_t svpwm_calc(
    const svpwm_input_t * const  p_in,
    svpwm_output_t      * const p_out );

#endif
