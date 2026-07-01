#include "fire_model_logger.hpp"

void FireModelLogger::start()
{
    logger->info("time,x,y,longitude,latitude,elevation,size,status");
}
