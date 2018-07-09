#include <Wire.h>
#include <GridEye.h>

GridEye myeye;

void setup(void)
{
  Wire.begin();
  myeye.setFramerate(10);

  Serial.begin(115200);
  while (!Serial) {
  }
}

int pixel[64];

void loop(void)
{
  delay(100);
  myeye.pixelOut(pixel);
  

   
  for (int i = 0; i < 64; i++) {
    Serial.write((pixel[i]     ) & 0xff);
    Serial.write((pixel[i] >> 8) & 0xff);
  }
}
