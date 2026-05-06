extern "C" {
    void run_benchmarks(void);
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 5000); 

    Serial.println("Stabilizing...");
    delay(1000); // Let the hardware settle

    // Optional: Warm up the cache by running once without timing
    // You could also do this inside the .c file.

    Serial.println("Starting measurement (Interrupts disabled)...");
    
    noInterrupts();
    run_benchmarks();
    interrupts();

    Serial.println("Benchmarks complete.");
}

void loop() {}