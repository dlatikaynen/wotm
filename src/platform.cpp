#include "inc/platform.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

namespace
{
    EM_JS(void, js_detect_browser, (char *out, int maxLen), {
        var ua = navigator.userAgent || "";
        var uaData = navigator.userAgentData;
        var name = "The browser";

        if (uaData && Array.isArray(uaData.brands))
        {
            for (var n = 0; n < uaData.brands.length; ++n)
            {
                var b = uaData.brands[n].brand;
                if (b && b.indexOf("Brand") === -1 && b !== "Chromium")
                {
                    name = b;
                    break;
                }
            }
        }

        if (name === "The browser")
        {
            if (ua.indexOf("Edg/") !== -1)
            {
                name = "Edge";
            }
            else if (ua.indexOf("OPR/") !== -1 || ua.indexOf("Opera") !== -1)
            {
                name = "Opera";
            }
            else if (ua.indexOf("Firefox/") !== -1) 
            {
                name = "Firefox";
            }
            else if (ua.indexOf("Chrome/") !== -1) 
            {
                name = "Chrome";
            }
            else if (ua.indexOf("Safari/") !== -1)
            {
                name = "Safari";
            }
        }

        var j = 0;
        for (var k = 0; k < name.length && j < maxLen - 1; ++k)
        {
            var c = name.charCodeAt(k);
            if (c < 128) 
            {
                HEAPU8[out + (j++)] = c;
            }
        }

        HEAPU8[out + j] = 0;
    });

    EM_JS(void, js_detect_device, (char *out, int maxLen), {
        var ua = navigator.userAgent || "";
        var uaData = navigator.userAgentData;
        var type = "computer";
        var lc = ua.toLowerCase();

        if (lc.indexOf("tablet") !== -1 || lc.indexOf("ipad") !== -1)
        {
            type = "tablet";
        }
        else if (uaData ? uaData.mobile
                        : (lc.indexOf("mobi") !== -1 ||
                           lc.indexOf("android") !== -1 ||
                           lc.indexOf("iphone") !== -1)
        ) {
            type = "phone";
        }

        var j = 0;
        for (var k = 0; k < type.length && j < maxLen - 1; ++k)
        {
            var c = type.charCodeAt(k);

            if (c < 128) 
            {
                HEAPU8[out + (j++)] = c;
            }
        }

        HEAPU8[out + j] = 0;
    });
}

ClientInfo detect_client_info()
{
    char buf[128];

    js_detect_browser(buf, sizeof(buf));
    std::string browser = buf;
    js_detect_device(buf, sizeof(buf));
    std::string deviceType = buf;

    return ClientInfo{ browser, deviceType };
}

#else
ClientInfo detect_client_info()
{
    return ClientInfo{ "The program", "computer" };
}
#endif
