/**
 * Diagnostic module
*/
#pragma once
#include "sdkconfig.h"
// Enable or Disable DEBUG_MODE implementation based on configuration
#ifdef CONFIG_TAPGATE_DEBUG_MODE

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace debug
{

constexpr uint32_t STACK_SIZE = 3072;

class Diagnostic
{
    public:
        static void start() {
            static Diagnostic instance;
            
            xTaskCreatePinnedToCore(
                &Diagnostic::task_trampoline, 
                "diag_task", 
                STACK_SIZE,
                &instance,
                1, 
                NULL, 
                1
            );
        }
    
    // Delete copy constructor and assignment operator
    Diagnostic(const Diagnostic&) = delete;
    Diagnostic& operator=(const Diagnostic&) = delete;

    // Delete move constructor and assignment operator
    Diagnostic(Diagnostic&&) = delete;
    Diagnostic& operator=(Diagnostic&&) = delete;
        
    private:  
        Diagnostic() = default;
    
        static void task_trampoline(void* pvParameters) {
            auto* self = static_cast<Diagnostic*>(pvParameters);
            self->run();
        }

        void run();
};

} // namespace debug

#endif // CONFIG_TAPGATE_DEBUG_MODE