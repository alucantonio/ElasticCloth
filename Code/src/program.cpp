/// @file

#include "program.hpp"

#define X_MIN           -1.0f
#define Y_MIN           -1.0f
#define X_MAX           1.0f
#define Y_MAX           1.0f
#define SIZE_X          10
#define SIZE_Y          10
#define NUM_POINTS      (SIZE_X*SIZE_Y)
#define DX              (float)((X_MAX-X_MIN)/SIZE_X)
#define DY              (float)((Y_MAX-Y_MIN)/SIZE_Y)
#define KERNEL_FILE1     "../../kernel/thekernel1.cl"
#define KERNEL_FILE2     "../../kernel/thekernel2.cl"

queue*  q1                = new queue();                                        // OpenCL queue.

kernel* k1                = new kernel();
kernel* k2                = new kernel();

point4* position          = new point4(NUM_POINTS);                             // Position.
color4* color             = new color4(NUM_POINTS);                             // Particle color.
float4* velocity          = new float4(NUM_POINTS);                             // Velocity.
float4* acceleration      = new float4(NUM_POINTS);                             // Acceleration.

point4* position_int      = new point4(NUM_POINTS);                             // Position (intermediate).
float4* velocity_int      = new float4(NUM_POINTS);                             // Velocity (intermediate).
float4* acceleration_int  = new float4(NUM_POINTS);                             // Acceleration (intermediate).

float4* gravity           = new float4(NUM_POINTS);                             // Gravity.
float4* stiffness         = new float4(NUM_POINTS);                             // Stiffness.
float4* resting           = new float4(NUM_POINTS);                             // Resting.
float4* friction          = new float4(NUM_POINTS);                             // Friction.
float4* mass              = new float4(NUM_POINTS);                             // Mass.

int1* index_PC            = new int1(NUM_POINTS);                               // Center particle.
int1* index_PR            = new int1(NUM_POINTS);                               // Right particle.
int1* index_PU            = new int1(NUM_POINTS);                               // Up particle.
int1* index_PL            = new int1(NUM_POINTS);                               // Left particle.
int1* index_PD            = new int1(NUM_POINTS);                               // Down particle.

float4* freedom           = new float4(NUM_POINTS);                             // Freedom/constrain flag.

float1* dt                = new float1(1);                                      // Time step.
float   simulation_time;                                                        // Simulation time.
int     time_step_number;                                                       // Time step index.

void setup()
{
  int i;
  int j;
  float x;
  float y;

  // Thickness, volume density, Young's modulus, viscosity
  float h = 1e-2;
  float rho = 1e3;
  float E = 1e5;
  float mu = 700.0;

  // Model parameters (mass, gravity, stiffness, damping)
  float m = rho*h*DX*DY;
  float g = 10.0f;
  float k = E*h*DY/DX;
  float c = mu*h*DX*DY;

  // Time step
  dt->x[0] = 0.8*sqrt(m/k);

  q1->init();                                                                    // Initializing OpenCL queue...

  k1->source_file = KERNEL_FILE1;                                               // Setting OpenCL kernel source file...
  k1->size = NUM_POINTS;                                                        // Setting kernel size...
  k1->dimension = 1;                                                            // Setting kernel dimension...
  k1->init();

  k2->source_file = KERNEL_FILE2;                                               // Setting OpenCL kernel source file...
  k2->size = NUM_POINTS;                                                        // Setting kernel size...
  k2->dimension = 1;                                                            // Setting kernel dimension...
  k2->init();

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////// Preparing arrays... /////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////
  y = Y_MIN;

  for (j = 0; j < SIZE_Y; j++)
  {
    x = X_MIN;

    for (i = 0; i < SIZE_X; i++)
    {
      // Setting "x" initial position...
      position->x[i + SIZE_X*j] = x;
      position->y[i + SIZE_X*j] = y;
      position->z[i + SIZE_X*j] = 0.0f;
      position->w[i + SIZE_X*j] = 1.0f;

      gravity->x[i + SIZE_X*j] = 0.0f;                                          // Setting "x" gravity...
      gravity->y[i + SIZE_X*j] = 0.0f;                                          // Setting "y" gravity...
      gravity->z[i + SIZE_X*j] = -g;                                         // Setting "z" gravity...
      gravity->w[i + SIZE_X*j] = 1.0f;                                          // Setting "w" gravity...

      stiffness->x[i + SIZE_X*j] = k;                                   // Setting "x" stiffness...
      stiffness->y[i + SIZE_X*j] = k;                                   // Setting "y" stiffness...
      stiffness->z[i + SIZE_X*j] = k;                                   // Setting "z" stiffness...
      stiffness->w[i + SIZE_X*j] = 1.0f;                                        // Setting "w" stiffness...

      resting->x[i + SIZE_X*j] = DX;                                            // Setting "x" resting position...
      resting->y[i + SIZE_X*j] = DX;                                            // Setting "y" resting position...
      resting->z[i + SIZE_X*j] = DX;                                            // Setting "z" resting position...
      resting->w[i + SIZE_X*j] = 1.0f;                                          // Setting "w" resting position...

      friction->x[i + SIZE_X*j] = c;                                        // Setting "x" friction...
      friction->y[i + SIZE_X*j] = c;                                        // Setting "y" friction...
      friction->z[i + SIZE_X*j] = c;                                        // Setting "z" friction...
      friction->w[i + SIZE_X*j] = 1.0f;                                         // Setting "w" friction...

      mass->x[i + SIZE_X*j] = m;                                             // Setting "x" mass...
      mass->y[i + SIZE_X*j] = m;                                             // Setting "y" mass...
      mass->z[i + SIZE_X*j] = m;                                             // Setting "z" mass...
      mass->w[i + SIZE_X*j] = 1.0f;                                             // Setting "w" mass...

      color->r[i + SIZE_X*j] = 1.0f;                                            // Setting "x" initial color...
      color->g[i + SIZE_X*j] = 0.0f;                                            // Setting "y" initial color...
      color->b[i + SIZE_X*j] = 0.0f;                                            // Setting "z" initial color...
      color->a[i + SIZE_X*j] = 1.0f;                                            // Setting "w" initial color...

      index_PC->x[i + SIZE_X*j] =  i + SIZE_X*j;

      freedom->x[i + SIZE_X*j] = 1.0f;
      freedom->y[i + SIZE_X*j] = 1.0f;
      freedom->z[i + SIZE_X*j] = 1.0f;
      freedom->w[i + SIZE_X*j] = 1.0f;

      if ((i != 0) && (i != (SIZE_X - 1)) && (j != 0) && (j != (SIZE_Y - 1)))   // When on bulk:
      {
        index_PR->x[i + SIZE_X*j] = (i + 1)  + SIZE_X*j;
        index_PU->x[i + SIZE_X*j] =  i       + SIZE_X*(j + 1);
        index_PL->x[i + SIZE_X*j] = (i - 1)  + SIZE_X*j;
        index_PD->x[i + SIZE_X*j] =  i       + SIZE_X*(j - 1);
      }

      else                                                                      // When on all borders:
      {
        gravity->x[i + SIZE_X*j] = 0.0f;                                        // Setting "x" gravity...
        gravity->y[i + SIZE_X*j] = 0.0f;                                        // Setting "y" gravity...
        gravity->z[i + SIZE_X*j] = 0.0f;                                        // Setting "z" gravity...
        gravity->w[i + SIZE_X*j] = 1.0f;                                        // Setting "w" gravity...

        freedom->x[i + SIZE_X*j] = 0.0f;
        freedom->y[i + SIZE_X*j] = 0.0f;
        freedom->z[i + SIZE_X*j] = 0.0f;
        freedom->w[i + SIZE_X*j] = 0.0f;
      }

      if ((i == 0) && (j != 0) && (j != (SIZE_Y - 1)))                          // When on left border (excluding extremes):
      {
        index_PR->x[i + SIZE_X*j] = (i + 1)  + SIZE_X*j;
        index_PU->x[i + SIZE_X*j] =  i       + SIZE_X*(j + 1);
        index_PL->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PD->x[i + SIZE_X*j] =  i       + SIZE_X*(j - 1);
      }

      if ((i == (SIZE_X - 1)) && (j != 0) && (j != (SIZE_Y - 1)))               // When on right border (excluding extremes):
      {
        index_PR->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PU->x[i + SIZE_X*j] =  i       + SIZE_X*(j + 1);
        index_PL->x[i + SIZE_X*j] = (i - 1)  + SIZE_X*j;
        index_PD->x[i + SIZE_X*j] =  i       + SIZE_X*(j - 1);
      }

      if ((j == 0) && (i != 0) && (i != (SIZE_X - 1)))                          // When on low border (excluding extremes):
      {
        index_PR->x[i + SIZE_X*j] = (i + 1)  + SIZE_X*j;
        index_PU->x[i + SIZE_X*j] =  i       + SIZE_X*(j + 1);
        index_PL->x[i + SIZE_X*j] = (i - 1)  + SIZE_X*j;
        index_PD->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
      }

      if ((j == (SIZE_Y - 1)) && (i != 0) && (i != (SIZE_X - 1)))               // When on high border (excluding extremes):
      {
        index_PR->x[i + SIZE_X*j] = (i + 1)  + SIZE_X*j;
        index_PU->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PL->x[i + SIZE_X*j] = (i - 1)  + SIZE_X*j;
        index_PD->x[i + SIZE_X*j] =  i       + SIZE_X*(j - 1);
      }

      if ((i == 0) && (j == 0))                                                 // When on low left corner:
      {
        index_PR->x[i + SIZE_X*j] = (i + 1)  + SIZE_X*j;
        index_PU->x[i + SIZE_X*j] =  i       + SIZE_X*(j + 1);
        index_PL->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PD->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
      }

      if ((i == (SIZE_X - 1)) && (j == 0))                                      // When on low right corner:
      {
        index_PR->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PU->x[i + SIZE_X*j] =  i       + SIZE_X*(j + 1);
        index_PL->x[i + SIZE_X*j] = (i - 1)  + SIZE_X*j;
        index_PD->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
      }

      if ((i == 0) && (j == (SIZE_Y - 1)))                                      // When on high left corner:
      {
        index_PR->x[i + SIZE_X*j] = (i + 1)  + SIZE_X*j;
        index_PU->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PL->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PD->x[i + SIZE_X*j] = i       + SIZE_X*(j - 1);
      }

      if ((i == (SIZE_X - 1)) && (j == (SIZE_Y - 1)))                           // When on high right corner:
      {
        index_PR->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PU->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PL->x[i + SIZE_X*j] = (i - 1)  + SIZE_X*j;
        index_PD->x[i + SIZE_X*j] =  i       + SIZE_X*(j - 1);
      }

      x += DX;
    }
    y += DY;
  }

  // Print info on time step (critical DT for stability)
  float cDT = sqrt(m/k);
  printf("Critical DT = %f\n", cDT);
  printf("Simulation DT = %f\n", dt->x[0]);

  // Set initial time to zero
  simulation_time = 0.0f;
  time_step_number = 0;

  position->init();                                                             // Initializing kernel variable...
  position_int->init();
  color->init();                                                                // Initializing kernel variable...
  velocity->init();                                                             // Initializing kernel variable...
  velocity_int->init();
  acceleration->init();                                                         // Initializing kernel variable...
  acceleration_int->init();
  gravity->init();                                                              // Initializing kernel variable...
  stiffness->init();                                                            // Initializing kernel variable...
  resting->init();                                                              // Initializing kernel variable...
  friction->init();                                                             // Initializing kernel variable...
  mass->init();                                                                 // Initializing kernel variable...
  index_PC->init();                                                             // Initializing kernel variable...
  index_PR->init();                                                             // Initializing kernel variable...
  index_PU->init();                                                             // Initializing kernel variable...
  index_PL->init();                                                             // Initializing kernel variable...
  index_PD->init();                                                             // Initializing kernel variable...
  freedom->init();                                                              // Initializing kernel variable...
  dt->init();

  position->set(k1, 0);                                                         // Setting kernel argument #0...
  color->set(k1, 1);                                                            // Setting kernel argument #1...
  position_int->set(k1, 2);                                                         // Setting kernel argument #0...
  velocity->set(k1, 3);                                                         // Setting kernel argument #3...
  velocity_int->set(k1, 4);                                                         // Setting kernel argument #3...
  acceleration->set(k1, 5);                                                     // Setting kernel argument #4...
  acceleration_int->set(k1, 6);                                                     // Setting kernel argument #4...
  gravity->set(k1, 7);                                                          // Setting kernel argument #5...
  stiffness->set(k1, 8);                                                        // Setting kernel argument #6...
  resting->set(k1, 9);                                                          // Setting kernel argument #7...
  friction->set(k1, 10);                                                         // Setting kernel argument #8...
  mass->set(k1, 11);                                                             // Setting kernel argument #9...
  index_PR->set(k1, 12);                                                        // Setting kernel argument #11...
  index_PU->set(k1, 13);                                                        // Setting kernel argument #12...
  index_PL->set(k1, 14);                                                        // Setting kernel argument #13...
  index_PD->set(k1, 15);                                                        // Setting kernel argument #14...
  freedom->set(k1, 16);                                                         // Setting kernel argument #15...
  dt->set(k1,17);

  position->set(k2, 0);                                                         // Setting kernel argument #0...
  color->set(k2, 1);                                                            // Setting kernel argument #1...
  position_int->set(k2, 2);                                                         // Setting kernel argument #0...
  velocity->set(k2, 3);                                                         // Setting kernel argument #3...
  velocity_int->set(k2, 4);                                                         // Setting kernel argument #3...
  acceleration->set(k2, 5);                                                     // Setting kernel argument #4...
  acceleration_int->set(k2, 6);                                                     // Setting kernel argument #4...
  gravity->set(k2, 7);                                                          // Setting kernel argument #5...
  stiffness->set(k2, 8);                                                        // Setting kernel argument #6...
  resting->set(k2, 9);                                                          // Setting kernel argument #7...
  friction->set(k2, 10);                                                         // Setting kernel argument #8...
  mass->set(k2, 11);                                                             // Setting kernel argument #9...
  index_PR->set(k2, 12);                                                        // Setting kernel argument #11...
  index_PU->set(k2, 13);                                                        // Setting kernel argument #12...
  index_PL->set(k2, 14);                                                        // Setting kernel argument #13...
  index_PD->set(k2, 15);                                                        // Setting kernel argument #14...
  freedom->set(k2, 16);
  dt->set(k2,17);                                                        // Setting kernel argument #15...
}

void save_vtk(float* position)
{
  printf("Action: Writing VTK file...");
  char preamble[100];
  snprintf(preamble, sizeof preamble, "# vtk DataFile Version 2.0\nCloth\n");
  write_file( "out.vtk", preamble);
  snprintf(preamble, sizeof preamble, "ASCII\n");
  write_file( "out.vtk", preamble);
  snprintf(preamble, sizeof preamble, "DATASET STRUCTURED_POINTS\n");
  write_file( "out.vtk", preamble);
  snprintf(preamble, sizeof preamble, "DIMENSIONS %i %i 1\n", SIZE_X, SIZE_Y);
  write_file( "out.vtk", preamble);
  snprintf(preamble, sizeof preamble, "ASPECT_RATIO 1 1 1\n");
  write_file( "out.vtk", preamble);
  snprintf(preamble, sizeof preamble, "ORIGIN 0 0 0\n");
  write_file( "out.vtk", preamble);
  snprintf(preamble, sizeof preamble, "POINT_DATA %i\n", NUM_POINTS);
  write_file( "out.vtk", preamble);
  snprintf(preamble, sizeof preamble, "SCALARS z-disp float 1\n");
  write_file( "out.vtk", preamble);
  snprintf(preamble, sizeof preamble, "LOOKUP_TABLE default\n");
  write_file( "out.vtk", preamble);

  for (int j = 0; j < SIZE_Y; j++)
  {
    for (int i = 0; i < SIZE_X; i++)
    {
      char buffer[20];
      snprintf(buffer, sizeof buffer, "%f ", position[4*(i+SIZE_X*j)+2]);
      write_file("out.vtk", buffer);
    }
    write_file("out.vtk","\n");
  }

  printf("DONE!\n");
}

void post_process(queue* q, point4* position)
{
  // Save vertical position of midpoint every 10 time steps
  if(time_step_number%10 == 0)
  {
    float t;
    float x[4*NUM_POINTS];
    float y;
    cl_int err;

    t = simulation_time;

    // Read vector of positions at time t
    err = clEnqueueReadBuffer(q->thequeue, position->buffer,
                              CL_TRUE, 0, 4*sizeof(cl_float)*NUM_POINTS,
                              x, 0, NULL, NULL);
    if(err < 0)
    {
      printf("\nError:  %s\n", get_error(err));
      exit(EXIT_FAILURE);
    }

    // Vertical position (z-component) of midpoint
    y = x[4*(NUM_POINTS-1)/2+2];

    // Format output string and write to CSV file
    char buffer [100];
    snprintf(buffer, sizeof buffer, "%f,%f\n", t, y);
    printf("T = %f\n", simulation_time);
    write_file("out.csv", buffer);

    if(time_step_number==10)
    {
      save_vtk(x);
    }
  }
}

void loop()
{
  position->push(q1, k1, 0);                                                     // Pushing kernel argument #0...
  color->push(q1, k1, 1);                                                        // Pushing kernel argument #1...
  position_int->push(q1, k1, 2);                                                     // Pushing kernel argument #0...
  velocity->push(q1, k1, 3);                                                     // Pushing kernel argument #3...
  velocity_int->push(q1, k1, 4);                                                     // Pushing kernel argument #3...
  acceleration->push(q1, k1, 5);                                                 // Pushing kernel argument #4...
  acceleration_int->push(q1, k1, 6);                                                 // Pushing kernel argument #4...
  gravity->push(q1, k1, 7);                                                      // Pushing kernel argument #5...
  stiffness->push(q1, k1, 8);                                                    // Pushing kernel argument #6...
  resting->push(q1, k1, 9);                                                      // Pushing kernel argument #7...
  friction->push(q1, k1, 10);                                                     // Pushing kernel argument #8...
  mass->push(q1, k1, 11);                                                         // Pushing kernel argument #9...
  index_PR->push(q1, k1, 12);                                                    // Pushing kernel argument #11...
  index_PU->push(q1, k1, 13);                                                    // Pushing kernel argument #12...
  index_PL->push(q1, k1, 14);                                                    // Pushing kernel argument #13...
  index_PD->push(q1, k1, 15);                                                    // Pushing kernel argument #14...
  freedom->push(q1, k1, 16);                                                     // Pushing kernel argument #15...
  dt->push(q1, k1, 17);

  k1->execute(q1, WAIT);

  k2->execute(q1, WAIT);

  post_process(q1, position);

  position->pop(q1, k2, 0);                                                      // Popping kernel argument #0...
  color->pop(q1, k2, 1);                                                         // Popping kernel argument #1...
  position_int->pop(q1, k2, 2);                                                      // Popping kernel argument #0...
  velocity->pop(q1, k2, 3);                                                      // Popping kernel argument #3...
  velocity_int->pop(q1, k2, 4);                                                      // Popping kernel argument #3...
  acceleration->pop(q1, k2, 5);                                                  // Popping kernel argument #4...
  acceleration_int->pop(q1, k2, 6);                                                  // Popping kernel argument #4...
  gravity->pop(q1, k2, 7);                                                       // Popping kernel argument #5...
  stiffness->pop(q1, k2, 8);                                                     // Popping kernel argument #6...
  resting->pop(q1, k2, 9);                                                       // Popping kernel argument #7...
  friction->pop(q1, k2, 10);                                                      // Popping kernel argument #8...
  mass->pop(q1, k2, 11);                                                          // Popping kernel argument #9...
  index_PR->pop(q1, k2, 12);                                                     // Popping kernel argument #11...
  index_PU->pop(q1, k2, 13);                                                     // Popping kernel argument #12...
  index_PL->pop(q1, k2, 14);                                                     // Popping kernel argument #13...
  index_PD->pop(q1, k2, 15);                                                     // Popping kernel argument #14...
  freedom->pop(q1, k2, 16);                                                      // Popping kernel argument #15...
  dt->pop(q1, k2, 17);

  plot(position, color, STYLE_POINT);                                           // Plotting points...

  // Update simulation time
  simulation_time += dt->x[0];
  time_step_number += 1;
}

void terminate()
{
  delete position;
  delete position_int;
  delete color;
  delete velocity;
  delete velocity_int;
  delete acceleration;
  delete acceleration_int;
  delete gravity;
  delete stiffness;
  delete resting;
  delete friction;
  delete mass;
  delete index_PD;
  delete index_PL;
  delete index_PR;
  delete index_PU;
  delete dt;

  delete q1;
  delete k1;
  delete k2;

  printf("All done!\n");
}
