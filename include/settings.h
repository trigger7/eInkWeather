struct WifiNetwork
{
    const char* ssid;
    const char* password;
};

WifiNetwork networks[1] = {
    {"my-ssid", "my-wifi-password"}
};

#define API_KEY "my-openweathermap-api-key"

#define LOCATION "Paradise,Us"
