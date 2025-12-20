/**
 * @file main.c
 * @brief Autonomous LIDAR Control System - Main Application
 * @author Paul Adutwum
 * @date 2025
 * 
 * =============================================================================
 *                          HOW LIDAR WORKS
 * =============================================================================
 * 
 * LIDAR (Light Detection and Ranging) measures distances by sending out
 * pulses and measuring the time for reflections to return.
 * 
 * PRINCIPLE: Time-of-Flight (ToF)
 * ─────────────────────────────────
 * 
 *     ┌─────────┐                    ┌─────────────┐
 *     │ EMITTER │ ──── Pulse ────▶   │   OBJECT    │
 *     │         │                    │  (Target)   │
 *     │ SENSOR  │ ◀── Reflection ─── │             │
 *     └─────────┘                    └─────────────┘
 *          │
 *          ▼
 *     Distance = (Speed × Time) / 2
 * 
 * For Ultrasonic (our implementation):
 *     Speed of Sound ≈ 343 m/s (at 20°C)
 *     Distance (cm) = (Echo Time in μs × 0.0343) / 2
 * 
 * For Optical LIDAR:
 *     Speed of Light ≈ 299,792,458 m/s
 *     Distance = (c × Δt) / 2
 * 
 * SCANNING MECHANISM
 * ─────────────────────
 * 
 *                   90°
 *                    │
 *            60°     │     120°
 *              ╲     │     ╱
 *               ╲    │    ╱
 *         30°    ╲   │   ╱    150°
 *           ╲     ╲  │  ╱     ╱
 *            ╲     ╲ │ ╱     ╱
 *             ╲     ╲│╱     ╱
 *    0° ───────●─────●─────●─────── 180°
 *            SERVO MOTOR
 *              (pivot)
 * 
 * The servo rotates the sensor through angles, creating a 2D distance map.
 * 
 * DATA OUTPUT FORMAT
 * ─────────────────────
 * Each measurement produces: (angle, distance)
 * These polar coordinates can be converted to Cartesian:
 *     x = distance × cos(angle)
 *     y = distance × sin(angle)
 * 
 * =============================================================================
 */

#include "../include/config.h"
#include "../include/hal.h"
#include "../drivers/uart.h"
#include "../drivers/pwm.h"
#include "../drivers/sensor.h"
#include "state_machine.h"

/*============================================================================
 *                         LIDAR SYSTEM CONSTANTS
 *============================================================================*/

/* Scan resolution: degrees per step (lower = higher resolution, slower) */
#define SCAN_RESOLUTION_DEG     1

/* Number of scan points in one sweep */
#define SCAN_POINTS             ((SERVO_MAX_ANGLE - SERVO_MIN_ANGLE) / SCAN_RESOLUTION_DEG + 1)

/* Minimum valid readings for object confirmation */
#define MIN_VALID_READINGS      3

/*============================================================================
 *                         LIDAR DATA STRUCTURES
 *============================================================================*/

/**
 * @brief Single LIDAR scan point (polar coordinates)
 */
typedef struct {
    uint16_t angle_deg;         /* Angle in degrees (0-180) */
    uint16_t distance_cm;       /* Distance in centimeters */
    int16_t  x_cm;              /* Cartesian X coordinate */
    int16_t  y_cm;              /* Cartesian Y coordinate */
    bool     valid;             /* Reading validity flag */
} lidar_point_t;

/**
 * @brief Complete scan data for one sweep
 */
typedef struct {
    lidar_point_t points[SCAN_POINTS];
    uint16_t      num_points;
    uint16_t      closest_distance;
    uint16_t      closest_angle;
    uint32_t      scan_time_ms;
    uint32_t      timestamp;
} lidar_scan_t;

/**
 * @brief Object detection result
 */
typedef struct {
    bool     detected;
    uint16_t distance_cm;
    uint16_t angle_deg;
    uint16_t width_deg;         /* Angular width of object */
    int16_t  center_x;
    int16_t  center_y;
} lidar_object_t;

/*============================================================================
 *                         GLOBAL VARIABLES
 *============================================================================*/

static lidar_scan_t current_scan;
static lidar_object_t detected_object;
static system_config_t config;

/* Trigonometry lookup tables for fast sin/cos */
static const int16_t sin_table[] = {
    0, 17, 35, 52, 70, 87, 105, 122, 139, 156,      /* 0-9° */
    174, 191, 208, 225, 242, 259, 276, 292, 309, 326, /* 10-19° */
    342, 358, 375, 391, 407, 423, 438, 454, 469, 485, /* 20-29° */
    500, 515, 530, 545, 559, 574, 588, 602, 616, 629, /* 30-39° */
    643, 656, 669, 682, 695, 707, 719, 731, 743, 755, /* 40-49° */
    766, 777, 788, 799, 809, 819, 829, 839, 848, 857, /* 50-59° */
    866, 875, 883, 891, 899, 906, 914, 921, 927, 934, /* 60-69° */
    940, 946, 951, 956, 961, 966, 970, 974, 978, 982, /* 70-79° */
    985, 988, 990, 993, 995, 996, 998, 999, 999, 1000, /* 80-89° */
    1000  /* 90° */
};

/*============================================================================
 *                         MATH HELPER FUNCTIONS
 *============================================================================*/

/**
 * @brief Fast sine approximation using lookup table
 * @param angle_deg Angle in degrees (0-180)
 * @return sin(angle) × 1000 (fixed-point)
 */
static int16_t fast_sin(uint16_t angle_deg) {
    if (angle_deg > 180) angle_deg = 180;
    
    if (angle_deg <= 90) {
        return sin_table[angle_deg];
    } else {
        return sin_table[180 - angle_deg];
    }
}

/**
 * @brief Fast cosine approximation using lookup table
 * @param angle_deg Angle in degrees (0-180)
 * @return cos(angle) × 1000 (fixed-point)
 */
static int16_t fast_cos(uint16_t angle_deg) {
    if (angle_deg > 180) angle_deg = 180;
    
    if (angle_deg <= 90) {
        return sin_table[90 - angle_deg];
    } else {
        return -sin_table[angle_deg - 90];
    }
}

/**
 * @brief Convert polar to Cartesian coordinates
 * @param distance Distance in cm
 * @param angle_deg Angle in degrees
 * @param x Pointer to store X coordinate
 * @param y Pointer to store Y coordinate
 */
static void polar_to_cartesian(uint16_t distance, uint16_t angle_deg, 
                                int16_t *x, int16_t *y) {
    /* x = distance × cos(angle), y = distance × sin(angle) */
    /* Using fixed-point math (÷1000 at the end) */
    *x = (int16_t)((int32_t)distance * fast_cos(angle_deg) / 1000);
    *y = (int16_t)((int32_t)distance * fast_sin(angle_deg) / 1000);
}

/*============================================================================
 *                         LIDAR CORE FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize LIDAR system
 */
static void lidar_init(void) {
    /* Initialize HAL */
    hal_system_init();
    
    /* Initialize UART for data output */
    uart_init(UART_BAUD_RATE);
    
    /* Initialize PWM for servo */
    pwm_init();
    servo_init(SERVO_PWM_PIN);
    
    /* Initialize sensors */
    sensors_init_all();
    
    /* Initialize alarm outputs */
    hal_gpio_init(LED_PIN, GPIO_MODE_OUTPUT);
    buzzer_init(BUZZER_PIN);
    motor_init(MOTOR_PIN);
    
    /* Initialize state machine */
    sm_init();
    
    /* Load default configuration */
    config.min_angle = SERVO_MIN_ANGLE;
    config.max_angle = SERVO_MAX_ANGLE;
    config.step_delay_ms = SERVO_STEP_DELAY_MS;
    config.default_threshold = DEFAULT_ALARM_THRESHOLD_CM;
    config.buzzer_enabled = true;
    config.night_mode_only = true;
    
    /* Move servo to start position */
    servo_set_angle(config.min_angle);
    hal_delay_ms(500);
    
    /* Clear scan data */
    current_scan.num_points = 0;
    current_scan.closest_distance = 0xFFFF;
    
    uart_tx_string("\r\n");
    uart_tx_string("╔══════════════════════════════════════════╗\r\n");
    uart_tx_string("║     AUTONOMOUS LIDAR CONTROL SYSTEM      ║\r\n");
    uart_tx_string("║         Time-of-Flight Ranging           ║\r\n");
    uart_tx_string("╚══════════════════════════════════════════╝\r\n");
    uart_tx_string("\r\n");
}

/**
 * @brief Take a single LIDAR measurement at current angle
 * @param point Pointer to store the measurement
 * @return true if valid reading, false otherwise
 */
static bool lidar_measure_point(lidar_point_t *point) {
    if (point == NULL) return false;
    
    /* Get current servo angle */
    point->angle_deg = servo_get_angle();
    
    /* Take distance measurement with averaging for accuracy */
    point->distance_cm = ultrasonic_measure_avg(DEBOUNCE_SAMPLES);
    
    /* Apply filtering */
    point->distance_cm = sensor_filter_distance(point->distance_cm);
    
    /* Validate reading */
    point->valid = sensor_validate_reading(point->distance_cm);
    
    if (point->valid) {
        /* Convert to Cartesian coordinates */
        polar_to_cartesian(point->distance_cm, point->angle_deg,
                          &point->x_cm, &point->y_cm);
    } else {
        point->x_cm = 0;
        point->y_cm = 0;
    }
    
    return point->valid;
}

/**
 * @brief Perform one complete LIDAR sweep
 * @param direction Scan direction (left-to-right or right-to-left)
 * @return Number of valid points collected
 */
static uint16_t lidar_sweep(scan_direction_t direction) {
    uint32_t start_time = hal_timer_get_ms();
    uint16_t point_index = 0;
    uint16_t valid_count = 0;
    
    current_scan.closest_distance = 0xFFFF;
    
    int16_t start_angle, end_angle, step;
    
    if (direction == SCAN_DIR_LEFT_TO_RIGHT) {
        start_angle = config.min_angle;
        end_angle = config.max_angle;
        step = SCAN_RESOLUTION_DEG;
    } else {
        start_angle = config.max_angle;
        end_angle = config.min_angle;
        step = -SCAN_RESOLUTION_DEG;
    }
    
    /* Sweep through all angles */
    for (int16_t angle = start_angle; 
         (step > 0) ? (angle <= end_angle) : (angle >= end_angle);
         angle += step) {
        
        /* Move servo to angle */
        servo_set_angle((uint16_t)angle);
        hal_delay_ms(config.step_delay_ms);
        
        /* Take measurement */
        lidar_point_t *point = &current_scan.points[point_index];
        
        if (lidar_measure_point(point)) {
            valid_count++;
            
            /* Track closest object */
            if (point->distance_cm < current_scan.closest_distance) {
                current_scan.closest_distance = point->distance_cm;
                current_scan.closest_angle = point->angle_deg;
            }
        }
        
        /* Send data via UART (protocol: angle,distance.) */
        uart_send_sensor_data(point->angle_deg, point->distance_cm);
        
        /* Check for alarm condition */
        uint16_t threshold = potentiometer_get_threshold(
            MIN_ALARM_THRESHOLD_CM, MAX_ALARM_THRESHOLD_CM);
        
        if (point->valid && point->distance_cm < threshold && 
            point->distance_cm > 0) {
            /* Object detected within threshold */
            handle_detection(point, threshold);
        }
        
        point_index++;
        if (point_index >= SCAN_POINTS) break;
    }
    
    current_scan.num_points = point_index;
    current_scan.scan_time_ms = hal_timer_get_ms() - start_time;
    current_scan.timestamp = hal_timer_get_ms();
    
    return valid_count;
}

/**
 * @brief Handle object detection event
 * @param point The point where object was detected
 * @param threshold Current alarm threshold
 */
static void handle_detection(lidar_point_t *point, uint16_t threshold) {
    /* Update detected object info */
    detected_object.detected = true;
    detected_object.distance_cm = point->distance_cm;
    detected_object.angle_deg = point->angle_deg;
    detected_object.center_x = point->x_cm;
    detected_object.center_y = point->y_cm;
    
    /* Trigger state machine event */
    sm_process_event(EVENT_TARGET_DETECTED);
    
    /* Activate LED */
    hal_gpio_write(LED_PIN, GPIO_STATE_HIGH);
    
    /* Activate motor (fan) */
    motor_on();
    
    /* Check if night mode for buzzer */
    uint16_t light_level = ldr_read();
    bool is_dark = ldr_is_dark(NIGHT_MODE_THRESHOLD);
    
    if (config.buzzer_enabled && (!config.night_mode_only || is_dark)) {
        buzzer_tone(1000);  /* 1kHz alarm tone */
    }
    
    /* Stay in alarm while object is close */
    while (detected_object.detected) {
        /* Re-measure at current position */
        uint16_t new_distance = ultrasonic_measure_avg(DEBOUNCE_SAMPLES);
        
        /* Update threshold from potentiometer */
        threshold = potentiometer_get_threshold(
            MIN_ALARM_THRESHOLD_CM, MAX_ALARM_THRESHOLD_CM);
        
        /* Send continuous updates */
        uart_send_sensor_data(point->angle_deg, new_distance);
        
        /* Check if object moved away */
        if (new_distance > threshold || new_distance == 0) {
            detected_object.detected = false;
            sm_process_event(EVENT_TARGET_LOST);
        }
        
        /* Brief delay to prevent overwhelming the serial */
        hal_delay_ms(ALARM_CHECK_INTERVAL_MS);
        
        /* Toggle buzzer for pulsing effect */
        buzzer_off();
        hal_delay_ms(50);
        if (config.buzzer_enabled && is_dark && detected_object.detected) {
            buzzer_tone(1000);
        }
    }
    
    /* Clear alarm outputs */
    hal_gpio_write(LED_PIN, GPIO_STATE_LOW);
    motor_off();
    buzzer_off();
}

/**
 * @brief Process and analyze scan data
 */
static void analyze_scan(void) {
    if (current_scan.num_points == 0) return;
    
    /* Find objects (clusters of close readings) */
    uint16_t cluster_start = 0;
    uint16_t cluster_count = 0;
    bool in_cluster = false;
    
    for (uint16_t i = 0; i < current_scan.num_points; i++) {
        lidar_point_t *point = &current_scan.points[i];
        
        if (point->valid && point->distance_cm < MAX_DISTANCE_CM) {
            if (!in_cluster) {
                cluster_start = i;
                in_cluster = true;
            }
            cluster_count++;
        } else {
            if (in_cluster && cluster_count >= MIN_VALID_READINGS) {
                /* Valid object cluster found */
                uint16_t cluster_end = i - 1;
                uint16_t width = current_scan.points[cluster_end].angle_deg - 
                                current_scan.points[cluster_start].angle_deg;
                
                DEBUG_PRINT("Object: %d-%d deg, width: %d deg\n",
                           current_scan.points[cluster_start].angle_deg,
                           current_scan.points[cluster_end].angle_deg,
                           width);
            }
            in_cluster = false;
            cluster_count = 0;
        }
    }
}

/*============================================================================
 *                         MAIN APPLICATION
 *============================================================================*/

/**
 * @brief Main entry point
 * 
 * LIDAR Operation Flow:
 * 1. Initialize all hardware (servo, sensor, outputs)
 * 2. Enter scanning state
 * 3. Sweep servo left-to-right, measuring distance at each angle
 * 4. Sweep servo right-to-left, measuring distance at each angle
 * 5. If object detected within threshold:
 *    - Activate LED indicator
 *    - Activate motor (fan)
 *    - If dark: Activate buzzer alarm
 *    - Stay locked until object moves away
 * 6. Repeat from step 3
 */
int main(void) {
    /* Initialize LIDAR system */
    lidar_init();
    
    /* Signal initialization complete */
    sm_process_event(EVENT_INIT_COMPLETE);
    
    uart_tx_string("[LIDAR] System initialized\r\n");
    uart_tx_string("[LIDAR] Starting scan...\r\n");
    
    /* Main control loop */
    scan_direction_t direction = SCAN_DIR_LEFT_TO_RIGHT;
    
    while (1) {
        /* Run state machine */
        system_state_t state = sm_run();
        
        /* Handle based on current state */
        switch (state) {
            case STATE_SCANNING:
                /* Perform LIDAR sweep */
                lidar_sweep(direction);
                
                /* Analyze collected data */
                analyze_scan();
                
                /* Alternate sweep direction */
                direction = (direction == SCAN_DIR_LEFT_TO_RIGHT) 
                          ? SCAN_DIR_RIGHT_TO_LEFT 
                          : SCAN_DIR_LEFT_TO_RIGHT;
                break;
                
            case STATE_TARGET_DETECTED:
            case STATE_ALARM_ACTIVE:
                /* Handled in handle_detection() */
                break;
                
            case STATE_ERROR:
                /* Flash LED rapidly to indicate error */
                hal_gpio_toggle(LED_PIN);
                hal_delay_ms(100);
                break;
                
            case STATE_CALIBRATING:
                /* Calibration mode */
                uart_tx_string("[LIDAR] Calibrating...\r\n");
                ultrasonic_calibrate(30);  /* Calibrate at 30cm */
                sm_process_event(EVENT_CALIBRATE_COMPLETE);
                break;
                
            default:
                break;
        }
    }
    
    return 0;
}

/*============================================================================
 *                         LIDAR THEORY REFERENCE
 *============================================================================*/

/**
 * LIDAR EQUATION:
 * ───────────────
 * 
 * The received signal power for LIDAR is given by:
 * 
 *     Pr = (Pt × Gt × σ × Ar) / (4π × R²)²
 * 
 * Where:
 *     Pr = Received power
 *     Pt = Transmitted power
 *     Gt = Transmitter gain
 *     σ  = Target cross-section (reflectivity)
 *     Ar = Receiver aperture area
 *     R  = Range to target
 * 
 * 
 * TIME-OF-FLIGHT CALCULATION:
 * ─────────────────────────────
 * 
 * For ultrasonic sensors (our implementation):
 * 
 *     Distance = (Echo_Time × Speed_of_Sound) / 2
 *     
 *     Speed of Sound at 20°C ≈ 343 m/s = 0.0343 cm/μs
 *     
 *     Distance (cm) = Echo_Time (μs) × 0.0343 / 2
 *                   = Echo_Time × 0.01715
 * 
 * 
 * ANGULAR RESOLUTION:
 * ─────────────────────
 * 
 * The angular resolution determines how precisely we can locate objects:
 * 
 *     Arc_Length = 2 × π × R × (θ / 360)
 * 
 * At 100cm range with 1° resolution:
 *     Arc_Length = 2 × 3.14159 × 100 × (1/360) ≈ 1.75 cm
 * 
 * This means at 1m distance, we can distinguish objects ~1.75cm apart.
 * 
 * 
 * COORDINATE TRANSFORMATION:
 * ────────────────────────────
 * 
 * Polar (r, θ) to Cartesian (x, y):
 *     x = r × cos(θ)
 *     y = r × sin(θ)
 * 
 * This allows mapping the scan data to a 2D grid for visualization.
 */

