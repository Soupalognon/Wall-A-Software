/*
 * Drv8262.hpp
 *
 *  Created on: Dec 8, 2025
 *      Author: thhoguin
 */

#ifndef MOTOR_DRV8262_HPP_
#define MOTOR_DRV8262_HPP_

#pragma once
#include <cstdint>

namespace DriversCustom {
namespace Motor {

/**
 * @brief Driver pour contrôler 2 moteurs avec le DRV8262 (H-Bridge dual)
 * 
 * Configuration matérielle:
 * - TIM1_CH1 (PE9): PWM Moteur Gauche (WHEEL_PWM1)
 * - TIM1_CH2 (PE11): PWM Moteur Droit (WHEEL_PWM2)
 * - PG3: IN1 (Direction moteur gauche bit 0)
 * - PG4: IN2 (Direction moteur gauche bit 1)
 * - PG5: IN3 (Direction moteur droit bit 0)
 * - PG6: IN4 (Direction moteur droit bit 1)
 * - PF1: SLEEP (Enable global)
 */
class Drv8262 {
public:
    Drv8262() = default;
    
    /**
     * @brief Initialise le driver (démarre les PWM)
     */
    void init();
    
    /**
     * @brief Contrôle le moteur gauche
     * @param duty Consigne de vitesse: -1.0 (arrière max) à +1.0 (avant max)
     */
    void setLeftDuty(float duty);
    
    /**
     * @brief Contrôle le moteur droit
     * @param duty Consigne de vitesse: -1.0 (arrière max) à +1.0 (avant max)
     */
    void setRightDuty(float duty);
    
    /**
     * @brief Active/désactive les moteurs
     * @param en true = moteurs activés, false = sleep mode
     */
    void enable(bool en);
    
    /**
     * @brief Arrête les deux moteurs (duty = 0)
     */
    void stop();
    
    /**
     * @brief Définit la vitesse des deux moteurs simultanément
     * @param leftDuty Duty du moteur gauche (-1..1)
     * @param rightDuty Duty du moteur droit (-1..1)
     */
    void setMotors(float leftDuty, float rightDuty);
};

} // namespace Motor
} // namespace DriversCustom

#endif /* MOTOR_DRV8262_HPP_ */
