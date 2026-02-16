#ifndef CALCULATEFPS_H
#define CALCULATEFPS_H 

double glfwGetTime(void);

static double calculateFPS()
{
    static double lastTime = glfwGetTime();
    static int nbFrames = 0;

    double currentTime = glfwGetTime();
    nbFrames++;

    static double fps = 0.0;
    if (currentTime - lastTime >= 1.0)
    {
        fps = double(nbFrames) / (currentTime - lastTime);
        nbFrames = 0;
        lastTime = currentTime;
    }
    return fps;
}

#endif
