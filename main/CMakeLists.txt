idf_component_register(SRCS "main.c" 
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES esp32_cert.pem esp32_key.pem html/on.html html/off.html
                    )