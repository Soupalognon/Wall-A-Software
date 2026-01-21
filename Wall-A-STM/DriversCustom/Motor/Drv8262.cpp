/*
 * Drv8262.cpp
 *
 *  Created on: Dec 8, 2025
 *      Author: thhoguin
 */

#include "Drv8262.hpp"
#include "main.h"
#include "Logger.hpp"
#include <algorithm>
#include <cmath>

// Déclaration externe du timer PWM configuré par CubeMX
extern TIM_HandleTypeDef htim1;

namespace DriversCustom {
namespace Motor {

// Configuration des GPIO pour les directions
// IN1/IN2 pour moteur gauche, IN3/IN4 pour moteur droit
// Logic: IN1=1, IN2=0 -> Forward | IN1=0, IN2=1 -> Backward | IN1=IN2 -> Brake

void Drv8262::init() {
    Libs::Utils::Logger::info("Drv8262: Initialisation...");
    
    // Démarrer les timers PWM
    // WHEEL_PWM1 (PE9 = TIM1_CH1) = moteur gauche
    // WHEEL_PWM2 (PE11 = TIM1_CH2) = moteur droit
    HAL_StatusTypeDef rc1 = HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    if (rc1 != HAL_OK) {
        Libs::Utils::Logger::error("Drv8262: ERREUR démarrage PWM CH1 (status=%d)", rc1);
        return;
    }
    Libs::Utils::Logger::info("Drv8262: PWM CH1 démarré OK");
    
    HAL_StatusTypeDef rc2 = HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    if (rc2 != HAL_OK) {
        Libs::Utils::Logger::error("Drv8262: ERREUR démarrage PWM CH2 (status=%d)", rc2);
        return;
    }
    Libs::Utils::Logger::info("Drv8262: PWM CH2 démarré OK");
    
    // Log de debug: config TIM1
    Libs::Utils::Logger::info("Drv8262: TIM1 ARR=%lu, PSC=%lu", htim1.Init.Period, htim1.Init.Prescaler);
    Libs::Utils::Logger::info("Drv8262: TIM1->CR1=0x%08lX, TIM1->CCER=0x%08lX", TIM1->CR1, TIM1->CCER);
    Libs::Utils::Logger::info("Drv8262: TIM1->CCMR1=0x%08lX (modes PWM)", TIM1->CCMR1);
    
    // Mettre les moteurs à l'arrêt initial
    stop();
    Libs::Utils::Logger::info("Drv8262: Moteurs à l'arrêt");
    
    // Activer le driver (sortir du mode SLEEP)
    enable(true);
    Libs::Utils::Logger::info("Drv8262: Driver activé (PRI_MOTOR_SLEEP = HIGH)");
    
    Libs::Utils::Logger::info("Drv8262: Init OK");
}

/**
 * @brief Convertit un duty cycle (-1..1) en valeur CCR pour le timer
 * @param duty Consigne normalisée
 * @return Valeur CCR (0 à ARR)
 */
static inline uint32_t dutyToCCR(float duty) {
    // Clamper entre -1 et 1
    duty = std::max(-1.0f, std::min(1.0f, duty));
    
    // Prendre la valeur absolue pour le PWM
    float absDuty = std::fabs(duty);
    
    // ARR du TIM1 est configuré à 65535 (16-bit)
    // Pour une résolution complète du timer
    const uint32_t maxCCR = 65535;
    
    return static_cast<uint32_t>(absDuty * maxCCR);
}

/**
 * @brief Configure les GPIO de direction pour le moteur gauche
 * @param duty Consigne: >0 = avant, <0 = arrière, 0 = frein
 * NOTE: Le moteur primaire n'a que des sorties PWM, pas de pins de direction.
 * La direction est contrôlée par la valeur du duty (implicite ou fixée au PCB).
 */
static void setLeftDirection(float duty) {
    // Pas de pins IN pour le primaire - la direction ne peut pas être controlée via GPIO
    // LOG seulement pour debug
    if (duty > 0.01f) {
        Libs::Utils::Logger::info("Drv8262: Moteur gauche AVANT (duty=%.2f)", duty);
    } else if (duty < -0.01f) {
        Libs::Utils::Logger::info("Drv8262: Moteur gauche ARRIERE (duty=%.2f)", duty);
    } else {
        Libs::Utils::Logger::info("Drv8262: Moteur gauche FREIN (duty=%.2f)", duty);
    }
}

/**
 * @brief Configure les GPIO de direction pour le moteur droit
 * @param duty Consigne: >0 = avant, <0 = arrière, 0 = frein
 * NOTE: Le moteur primaire n'a que des sorties PWM, pas de pins de direction.
 * La direction est contrôlée par la valeur du duty (implicite ou fixée au PCB).
 */
static void setRightDirection(float duty) {
    // Pas de pins IN pour le primaire - la direction ne peut pas être controlée via GPIO
    // LOG seulement pour debug
    if (duty > 0.01f) {
        Libs::Utils::Logger::info("Drv8262: Moteur droit AVANT (duty=%.2f)", duty);
    } else if (duty < -0.01f) {
        Libs::Utils::Logger::info("Drv8262: Moteur droit ARRIERE (duty=%.2f)", duty);
    } else {
        Libs::Utils::Logger::info("Drv8262: Moteur droit FREIN (duty=%.2f)", duty);
    }
}

void Drv8262::setLeftDuty(float duty) {
    // Log direction (pas de GPIO de direction pour primaire)
    setLeftDirection(duty);
    
    // Configurer le PWM (valeur absolue) sur WHEEL_PWM1 (TIM1_CH1 = PE9)
    uint32_t ccr = dutyToCCR(duty);
    
    // Écrire dans CCR1
    TIM1->CCR1 = ccr;
    
    // Vérifier que ça a été écrit
    uint32_t verif = TIM1->CCR1;
    Libs::Utils::Logger::info("Drv8262: CH1 CCR = %lu (duty=%.2f, verif=%lu)", ccr, duty, verif);
}

void Drv8262::setRightDuty(float duty) {
    // Log direction (pas de GPIO de direction pour primaire)
    setRightDirection(duty);
    
    // Configurer le PWM (valeur absolue) sur WHEEL_PWM2 (TIM1_CH2 = PE11)
    uint32_t ccr = dutyToCCR(duty);
    
    // Écrire dans CCR2
    TIM1->CCR2 = ccr;
    
    // Vérifier que ça a été écrit
    uint32_t verif = TIM1->CCR2;
    Libs::Utils::Logger::info("Drv8262: CH2 CCR = %lu (duty=%.2f, verif=%lu)", ccr, duty, verif);
}

void Drv8262::enable(bool en) {
    // Contrôler la pin SLEEP (PF1 = PRI_MOTOR_SLEEP)
    // HIGH = actif, LOW = sleep mode
    GPIO_PinState state = en ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(PRI_MOTOR_SLEEP_GPIO_Port, PRI_MOTOR_SLEEP_Pin, state);
    Libs::Utils::Logger::info("Drv8262: PRI_MOTOR_SLEEP = %s", en ? "HIGH" : "LOW");
}

void Drv8262::stop() {
    setLeftDuty(0.0f);
    setRightDuty(0.0f);
}

void Drv8262::setMotors(float leftDuty, float rightDuty) {
    setLeftDuty(leftDuty);
    setRightDuty(rightDuty);
}

} // namespace Motor
} // namespace DriversCustom

