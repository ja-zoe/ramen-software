void setup() {

Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }

    Wire.begin();

    sen5x.begin(Wire);

    uint16_t error;
    char errorMessage[256];
    error = sen5x.deviceReset();

    //retrying the function to make it work. if it doesn't it will print error
    int i =0;
    while (error && i < 11) { 
        error  = sen5x.deviceReset(); 
        i++;    }  
    if (error){
        Serial.print("Error trying to execute deviceReset(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage); }
    
    
    
    // Start Measurement
    //retrying the function to make it work. if it doesn't it will print error
    error = sen5x.startMeasurement();
    int j =0;
    while (error && j < 11) { 
        error  = sen5x.startMeasurement();
        j++;    }  
    if (error){
        Serial.print("Error trying to execute startMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage); }
    }



void loop() {
  // put your main code here, to run repeatedly:

}
