
  var config = {
    address: "localhost",
    port: 8080,
    basePath: "/",
    ipWhitelist: [
      "127.0.0.1",
      "::ffff:127.0.0.1",
      "::1",
      "192.168.4.130",
      "192.168.4.71"
    ],
    cors: "disabled",
    language: "en",
    logLevel: [
      "INFO",
      "LOG",
      "WARN",
      "ERROR"
    ],
    timeFormat: 12,
    units: "imperial",
    locale: "en-US",
    useHttps: false,
    ignoreXOriginHeader: false,
    ignoreContentSecurityPolicy: false,
    reloadAfterServerRestart: false,
    corsDomainWhitelist: [],
    modules: [
  /*    {
        module: "updatenotification",
        position: "top_bar",
        order: "*",
        disabled: false,
        hiddenOnStartup: false,
        configDeepMerge: false,
        animateIn: "None",
        animateOut: "None"
      }, */
      {
        module: "clock",
        position: "top_left",
        order: "*",
        disabled: false,
        label: "instance 1",
        hiddenOnStartup: false,
        configDeepMerge: false,
        dateFormat: "ddd, ll", // didnt work changed in clock.js
        animateIn: "None",
        animateOut: "None",
        index: 0
      },
      {
        module: "calendar",
        header: "US Holidays",
        position: "top_left",
        config: {
          maximumEntries: 3,
          calendars: [
            {
              symbol: false,  
              // symbol: "calendar-check",
              url: "https://ics.calendarlabs.com/76/mm3137/US_Holidays.ics",
             // symbolClassName: "fas fa-fw fa-",
              auth: {
                password: "",
                method: "basic"
              },
              broadcastPastEvents: false
            }
          ],
         
        },
        order: "*",
        disabled: false,
        label: "instance 1",
        hiddenOnStartup: false,
        configDeepMerge: false,
        animateIn: "None",
        animateOut: "None",
        index: 0
      },
      
      {
            
            module: 'MMM-ImageSlideshow',
            position: 'middle_center',
            config: {
                // imagePaths: ['modules/MMM-ImageSlideshow/images'],
                slideshowSpeed: 60000,
                fixedImageWidth: 800,
                fixedImageHeight: 700,
                randomizeImageOrder: true,
            }
        }, 
      
      {
        module: "MMM-MoonPhase",
        position: "top_left",
        config: {
          basicColor: "white",
          size: 90,
          alpha: 0.7,
          riseAndSet: {
            display: true,
            lon: -108.35093,
            lat: 39.110288,
            gmtOffset: -6
          }
        },
        order: "*",
        disabled: false,
        hiddenOnStartup: false,
        configDeepMerge: false,
        animateIn: "None",
        animateOut: "None"
      },
      {
		module: "MMM-APOD",
		position: "bottom_left",
		config: {
			appid: "ETycVoZz3E4TSi7ldn1ehmUUPJCpTD8MzR4LJsWk", // NASA API key (api.nasa.gov)
            initialLoadDelay: 3000, // Delays the aAPI call for 3 seconds
			maxMediaWidth: 290,
			maxMediaHeight: 240
          }
       }, 
       
 {
		module: "MMM-APOD2",
		position: "top_center",
		config: {
			appid: "ETycVoZz3E4TSi7ldn1ehmUUPJCpTD8MzR4LJsWk", // NASA API key (api.nasa.gov)
            showDescription: true,
            useShortDescription: false,
			maxMediaWidth: 1000,
			maxMediaHeight: 500
          }
       },      
      
      
     {
        module: "weather",
        position: "top_right",
        config: {
          units: "config.units",     
          weatherProvider: "weathergov",
          apiBase: "https://api.weather.gov/points/",
          lat: 39.11026,
          lon: -108.35092,
          showHumidity: "temp",
          showPrecipitationProbability: true,
          showPrecipitationAmount: true
        },
       
      }, 
      
/*    {
        module: "weather",
        position: "top_right",
        header: "Current___",
        config: {
          type: "current",
          units: "config.units",  
          weatherProvider: "weathergov",
          apiBase: "https://api.weather.gov/points/",
          lat: 39.11026,
          lon: -108.35092,
          currentForecastHours: 6,
          showHumidity: "temp",
          showPrecipitationAmount: true,
          showPrecipitationProbability: true
	      themeDir: "../../../modules/MMT-WeatherSkycons",
  		  themeCustomScripts: ["skycons.js", "weathertheme.js"],

          
        },

      }, */       
        
 /*   {
        module: "weather",
        position: "top_right",
        header: "Forecast___",
        config: {
          type: "forecast",  
          weatherProvider: "weathergov",
          apiBase: "https://api.weather.gov/points/",
          lat: 39.11026,
          lon: -108.35092,
          colored: true,
          themeDir: "../../../modules/MMT-WeatherSkycons",
  		  themeCustomScripts: ["skycons.js", "weathertheme.js"]
          
          
        },

      },   */   
      
      
      
           
      
      {
        module: "weather",
        position: "top_right",
        header: "Weather Forecast",
        config: {
            initialLoadDelay: 1000,
            type: "daily",
            weatherProvider: "weathergov",
            apiBase: "https://api.weather.gov/points/",
            lat: 39.11026,
            lon: -108.35092,
            tableClass: "medium",
            colored: true,
            ignoreToday: true     
        },
              
      },  
      
      




      

        {
  module: 'MMM-MQTT',
  position: 'bottom_right',
  header: 'OUTSIDE',
  config: {
    logging: false,
    useWildcards: false,
    bigMode: false, // Set to true to display big numbers with label above
    mqttServers: [
      {
        address: 'localhost',  // Server address or IP address
        port: '1883',          // Port number if other than default

    
        
        user: 'user',          // Leave out for no user
        password: 'password',      // Leave out for no password
        subscriptions: [
            
           {
            topic: 'sensors/am2301b/temperature', // Topic to look for
            label: 'Frontyard temp', // Displayed in front of value
            suffix: '°F',         // Displayed after the value
            decimals: 1,          // Round numbers to this number of decimals
            sortOrder: 40,        // Can be used to sort entries in the same table
            maxAgeSeconds: 720,    // Reduce intensity if value is older
            broadcast: true,      // Broadcast messages to other modules
            colors: [             // Value dependent colors
              { upTo: -20, value: "blue", label: "blue", suffix: "blue" },
              { upTo: 32, value: "#00ccff", label: "#00ccff", suffix: "#00ccff" },
              { upTo: 68, value: "yellow"},
              { upTo: 80, label: "3489eb", suffix: "green" },
              { upTo: 120, label: "red" }, // The last one is used for higher values too
            ],
          },          
          
           {
            topic: 'sensors/am2301b/humidity',
            label: 'Fronyard Humidity',
            suffix: '%',
            decimals: 1,
            decimalSignInMessage: ",", // If the message decimal point is not "."
            sortOrder: 50,
            maxAgeSeconds: 720
            
          },    
            
            
          {
            topic: 'sensors/bme280/temperature', // Topic to look for
            label: 'Backyard Temp', // Displayed in front of value
            suffix: '°F',         // Displayed after the value
            decimals: 1,          // Round numbers to this number of decimals
            sortOrder: 10,        // Can be used to sort entries in the same table
            maxAgeSeconds: 720,    // Reduce intensity if value is older
            broadcast: true,      // Broadcast messages to other modules
            colors: [             // Value dependent colors
              { upTo: -20, value: "blue", label: "blue", suffix: "blue" },
              { upTo: 32, value: "#00ccff", label: "#00ccff", suffix: "#00ccff" },
              { upTo: 68, value: "yellow"},
              { upTo: 80, label: "3289eb", suffix: "green" },
              { upTo: 120, label: "red" }, // The last one is used for higher values too
            ],
          },
          {
            topic: 'sensors/bme280/humidity',
            label: 'Backyard Humidity',
            suffix: '%',
            decimals: 0,        
            sortOrder: 20,
            maxAgeSeconds: 720
          },
          
           {
            topic: 'sensors/bme280/pressure',
            label: 'Backyard Pressure',
            suffix: 'hPa',
            decimals: 1,
            decimalSignInMessage: ",", // If the message decimal point is not "."
            sortOrder: 30,
            maxAgeSeconds: 720
            
          }
          
         
          
          
        ]
      }
    ],
  }
},

    {
        module: 'MMM-pages',
        config: {
                modules: [
                    ['clock', "calendar", 'MMM-ImageSlideshow', 'MMM-MoonPhase', 'MMM-APOD', 'weather', 'MMM-MQTT'],
                    ['MMM-APOD2'],
                 ],
                fixed: ['alert', 'MMM-page-indicator']
        }
    }, 
    
    {
        module: 'MMM-page-indicator',
        position: 'bottom_bar',
        config: {
            pages: 2,
        }
    },
{
      module: "MMM-Buttons",
    //  position: "bottom_left",     // uncomment to see debug
      config: {
        bounceTimeout: 30,
        minLongPressTime: 1005,  
        buttons: [
          {
            pin: 16,
            name: "page_increment",
            longPress: [
              {
                
                notification: "PAGE_INCREMENTT",
                payload: {action: 1}
              }
            ],
            shortPress: [
              {
                notification: "PAGE_INCREMENT",
                payload: {action: 1}
              }
            ]
          },        
        ]
      }
    },
    

      {
        module: "alert",
        order: "*",
        disabled: false,
        label: "instance 1",
        hiddenOnStartup: false,
        configDeepMerge: false,
        animateIn: "None",
        animateOut: "None",
        index: 0
      }
    ]
  };

/*************** DO NOT EDIT THE LINE BELOW ***************/
if (typeof module !== "undefined") {module.exports = config;}
