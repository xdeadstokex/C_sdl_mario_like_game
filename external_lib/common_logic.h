#ifndef COMMON_LOGIC_H
#define COMMON_LOGIC_H

//###############################################
//####               CONST                   ####
//###############################################
#define PI 3.14159265358979323846
#define PI2 2 * 3.14159265358979323846
#define PI_HALF 0.5 * 3.14159265358979323846



//###############################################


//###############################################
//####           COLLISION                   ####
//###############################################
static inline int check_point_in_box_2d(
double x0, double y0,
double x1, double y1, double w1, double h1){
if(x0 < x1 || x0 > x1 + w1
|| y0 < y1 || y0 > y1 + h1){ return 0; }
return 1;
}


static inline int check_two_box_2d_hit_basic(
register double x0, register double y0, register double w0, register double h0,
register double x1, register double y1, register double w1, register double h1){
if(w0 <= 0 || w1 <= 0 || h0 <= 0 || h1 <= 0){ return 0; }
// check boxes x range non-collision
if(x1 > x0 + w0){ return 0; }
if(x1 + w1 < x0){ return 0; }
// check boxes y range non-collision
if(y1 > y0 + h0){ return 0; }
if(y1 + h1 < y0){ return 0; }

return 1;// boxes collide (overlap on both x and y axes)
}


static inline int check_two_box_2d_hit_centralized(
register double x0, register double y0, register double w0, register double h0,
register double x1, register double y1, register double w1, register double h1){
return check_two_box_2d_hit_basic(
x0 - w0 / 2, y0 - h0 / 2, w0, h0,
x1 - w1 / 2, y1 - h1 / 2, w1, h1);
}



//###############################################
//####           ANGLE                       ####
//###############################################
double get_angle_2d(double start_x, double start_y, double end_x, double end_y){
double delta_x = end_x - start_x;
double delta_y = end_y - start_y;
double angle;
if(delta_x != 0) angle = atan(delta_y / delta_x);
else angle = (end_y > start_y) ? PI_HALF : -PI_HALF;

if(delta_x >= 0){
if(delta_y >= 0){
angle += 0;
}
}

if(delta_x < 0){
if(delta_y >= 0){
angle -= PI;
angle += PI2;
}
}

if(delta_x < 0){
if(delta_y < 0){
angle += PI;
}
}

if(delta_x >= 0){
if(delta_y < 0){
angle += PI2;
}
}

return angle;
}



double adjust_angle(double angle){
if(angle > PI2) return angle - PI2;
if(angle < 0) return angle + PI2;
return angle;
}


double get_opposite_angle(double angle){
return adjust_angle(angle + PI);
}


double get_delta_2d(double x, double y, double end_x, double end_y){
return sqrt((end_x - x) * (end_x - x) + (end_y - y) * (end_y - y));
}


/*
a = a ^ b // a ^ b (yes the value of both a and b in a sense)
b = a ^ b // b = (a ^ b) ^ b ( do the logic yourself)
a = a ^ b // same as above

*/


/*
double end_point_x(double angle, double x, double len){
return len * cos(angle) + x;
}

double end_point_y(double angle, double y, double len){
return len * sin(angle) + y;
}
*/









/*
short line_collision(pos_2d line1[2], pos_2d line2[2]) {
// Calculate direction vectors
double dx1 = line1[1].x - line1[0].x;
double dy1 = line1[1].y - line1[0].y;
double dx2 = line2[1].x - line2[0].x;
double dy2 = line2[1].y - line2[0].y;
  
// Calculate the determinant
double det = dx1 * dy2 - dy1 * dx2;
    
// If determinant is zero, lines are parallel or collinear
if (det == 0) return 0;
    
// Calculate parameters for the intersect point
double s = ((line2[0].x - line1[0].x) * dy2 - (line2[0].y - line1[0].y) * dx2) / det;
double t = ((line1[0].x - line2[0].x) * dy1 - (line1[0].y - line2[0].y) * dx1) / -det;
    
// Check if intersection point is within both line segments
if (s >= 0 && s <= 1 && t >= 0 && t <= 1) return 1; // Lines intersect
return 0; // No intersection
}



// alr selective non same vertices line in 6*5 check
short quad_shape_undented(pos_2d pos[4], short order[4]){
pos_2d line1[2] = {pos[0], pos[1]};
pos_2d line2[2] = {pos[2], pos[3]};
if(line_collision(line1, line2) == 1){
order[0] = 0;
order[1] = 2;
order[2] = 1;
order[3] = 3;
return 1;
}

line1[0] = pos[0];
line1[1] = pos[2];
line2[0] = pos[1];
line2[1] = pos[3];
if(line_collision(line1, line2) == 1){
order[0] = 0;
order[1] = 1;
order[2] = 2;
order[3] = 3;
return 2;
}

line1[0] = pos[0];
line1[1] = pos[3];
line2[0] = pos[1];
line2[1] = pos[2];
if(line_collision(line1, line2) == 1){
order[0] = 0;
order[1] = 1;
order[2] = 3;
order[3] = 2;
return 3;
}

return 0;
}
*/
#endif