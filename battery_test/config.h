/* Application Logic Config*/
#define SLEEP_INTERVAL  5 // Time ESP32 will go to sleep (in seconds)
#define SAMPLES_PER_TRANSMISSION 2 // Amount of samples-- or sleep cycles --the sensor will take before transmitting

/*Wifi Config*/
#define WIFI_SSID "Julian's iPhone"
#define WIFI_PASSWORD "pinetree"

/*Supabase Database Config*/
#define SUPABASE_URL "https://sgryddverfpfapzhrnfu.supabase.co"
#define SUPABASE_ANON_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InNncnlkZHZlcmZwZmFwemhybmZ1Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3Njk4MzM2MzUsImV4cCI6MjA4NTQwOTYzNX0.a6EGk-wHqlWUJbTwOGxrt60jKPt-6pGCi4cp1BjmC0E"
#define DB_TABLE "Battery_Test_1"

#define EMAIL "esp32user@gmail.com"
#define PASSWORD "esp32c3-xiao"