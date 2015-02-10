#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <libc.h>
#ifndef PI
#define PI M_PI
#endif PI
#define TWOPI 6.28318530717059647602
#define HPI M_PI_2
#define RADTODEG 57.29577951
#define DEGTORAD 0.017453292

float x0;
float z0;
float angmin;
float angmax;
const char progname[] = "frustumtest";

void setfrustum(float x, float z, float ang, float fov, float rad);
int insidefrustum(float x, float z);

main()
{
    float x,z,ang,fov,rad;
    char input[256];

newfrustum:

    printf("Enter x, z, ang, fov, rad: ");
    scanf("%g %g %g %g %g",&x,&z,&ang,&fov,&rad);

    setfrustum(x,z,ang,fov,rad);

    while (1) {
        printf("Enter x, z: ");
        scanf("%s",input);
        if (!strcmp(input,"quit"))
            break;
        if (!strcmp(input,"new"))
            goto newfrustum;
        sscanf(input,"%g",&x);
        scanf("%g",&z);
        if (insidefrustum(x,z))
            printf("(%g,%g) is inside the frustum\n",x,z);
        else
            printf("(%g,%g) is outside the frustum\n",x,z);
    }
    exit(0);
}

void setfrustum(float x, float z, float ang, float fov, float rad)
{
    x0 = x + rad * sin(ang*DEGTORAD) / sin(fov*0.5*DEGTORAD);
    z0 = z + rad * cos(ang*DEGTORAD) / sin(fov*0.5*DEGTORAD);
    angmin = fmod((ang - 0.5*fov)*DEGTORAD,TWOPI);
    if (fabs(angmin) > PI) angmin -= (angmin > 0.0) ? TWOPI : (-TWOPI);
    angmax = fmod((ang + 0.5*fov)*DEGTORAD,TWOPI);
    if (fabs(angmax) > PI) angmax -= (angmin > 0.0) ? TWOPI : (-TWOPI);
    printf("x0, z0, angmin, angmax = %g, %g, %g, %g\n",x0,z0,angmin,angmax);
}


int insidefrustum(float x, float z)
{
    float ang;

    ang = atan2(x0-x,z0-z);
    printf("ang = %g\n",ang);
    if (angmin < angmax) {
        if (ang < angmin)
        {
            return 0;
        }
        else if (ang > angmax)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else {
        if (ang > angmin)
        {
            return 1;
        }
        else if (ang < angmax)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}
