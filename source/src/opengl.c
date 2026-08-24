#include <GL/glut.h>      
#include <stdio.h>        
#include <stdlib.h>       
#include <fcntl.h>        
#include <unistd.h>       
#include <string.h>       

// Game state variables
int team1_effort = 0, team2_effort = 0;     // current effort values for each team
int team1_score = 0, team2_score = 0;       // score counters
int round_number = 0;                       // current round number
int round_winner = -1, final_winner = -1;   // winner indicators ,-1 = no winner yet 
int fifo_fd;                                // file descriptor for FIFO 'named pipe'

// Function to draw text on the screen at (x, y) with RGB color
void display_text(float x, float y, const char *string, float r, float g, float b) {
    glColor3f(r, g, b);                     // set color
    glRasterPos2f(x, y);                   // set position
    while(*string)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *string++); // draw each character
}

// main display function that draws the game scene
void display() {
    glClear(GL_COLOR_BUFFER_BIT);          // clear the screen
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);   // set background color (dark gray)

    // draw the rope as a line
    glLineWidth(8.0f);                      // set line thickness
    glBegin(GL_LINES);                     
        glColor3f(0.7f, 0.5f, 0.3f);       // rope color 'brown'
        glVertex2f(-0.8f, 0.0f);           // start of rope
        glVertex2f(0.8f, 0.0f);            // end of rope
    glEnd();

    // calculate how much the rope is pulled based on efforts
    float base_position = ((float)(team2_effort - team1_effort)) / 1000.0f;

    // clamp the rope position within limits
    if (base_position > 0.5f) base_position = 0.5f;
    else if (base_position < -0.5f) base_position = -0.5f;

    // draw Team first players as blue dots on rope
    for (int i = 0; i < 4; i++) {
        float offset = -0.1f * (i + 1);    // position each player to left
        glPointSize(15);                   // set dot size
        glBegin(GL_POINTS);
            glColor3f(0.0f, 0.8f, 1.0f);   // blue 
            glVertex2f(base_position + offset, 0.0f); // position for Player 
        glEnd();
    }

    // draw Team second players as orange dots on rope
    for (int i = 0; i < 4; i++) {
        float offset = 0.1f * (i + 1);     // position each player to right
        glPointSize(15);
        glBegin(GL_POINTS);
            glColor3f(1.0f, 0.5f, 0.0f);   // orange 
            glVertex2f(base_position + offset, 0.0f);
        glEnd();
    }

    // display team names
    display_text(-0.9f, 0.05f, "Team 1", 0.0f, 0.8f, 1.0f); // blue team
    display_text(0.75f, 0.05f, "Team 2", 1.0f, 0.5f, 0.0f); // orange team

    char buffer[256];  // buffer for formatting strings

    // show the current round number
    sprintf(buffer, "Round: %d", round_number);
    display_text(-0.15f, 0.8f, buffer, 1.0f, 1.0f, 1.0f);

    // show effort values for both teams
    sprintf(buffer, "Team 1 Effort: %d", team1_effort);
    display_text(-0.8f, 0.6f, buffer, 0.0f, 0.8f, 1.0f);

    sprintf(buffer, "Team 2 Effort: %d", team2_effort);
    display_text(0.3f, 0.6f, buffer, 1.0f, 0.5f, 0.0f);

    // show score values for both teams
    sprintf(buffer, "Score - Team 1: %d", team1_score);
    display_text(-0.8f, 0.4f, buffer, 0.0f, 0.8f, 1.0f);

    sprintf(buffer, "Score - Team 2: %d", team2_score);
    display_text(0.3f, 0.4f, buffer, 1.0f, 0.5f, 0.0f);

    // if there's a round winner, show the message
    if(round_winner != -1){
        sprintf(buffer, "Team %d Wins Round %d!", round_winner+1, round_number);
        display_text(-0.3f, -0.4f, buffer, 1.0f, 1.0f, 0.0f); // Yellow text
    }

    // if there a final winner show message
    if(final_winner != -1){
        sprintf(buffer, "Team %d Wins the Game!", final_winner);
        display_text(-0.35f, -0.6f, buffer, 0.0f, 1.0f, 0.0f); // green text
    }

    glutSwapBuffers(); // swap front and back buffers (for smooth animation)
}

//  function runs in idle time, constantly checking for new data from FIFO
void idle() {
    char buf[256];                          // buffer to read data
    ssize_t n = read(fifo_fd, buf, sizeof(buf)-1); // read from FIFO pipe
    if (n > 0) {
        buf[n] = '\0';                      // null terminate the string
        // parse data into variables
        sscanf(buf, "%d %d %d %d %d %d %d", &round_number, &team1_effort, &team2_effort,
               &team1_score, &team2_score, &round_winner, &final_winner);
    }
}

// timer callback to refresh screen regularly
void timer(int v) {
    glutPostRedisplay();                   // request new frame to be drawn
    glutTimerFunc(100, timer, v);          // call glutTimerFunc again after 100 ms
}

// setup OpenGL window and event handlers
void init_window(int argc, char **argv) {
    glutInit(&argc, argv);                                // initialize GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);          // use double buffering and RGB color
    glutInitWindowSize(900, 500);                         // set window size
    glutInitWindowPosition(100, 100);                     // set window position
    glutCreateWindow("Rope Pulling Game");                // create the window with title
    glutDisplayFunc(display);                             // set display callback
    glutIdleFunc(idle);                                   // set idle callback
    glutTimerFunc(100, timer, 0);                         // start timer loop
}

// entry point to the OpenGL loop
void run_opengl_loop() {
    fifo_fd = open("/tmp/rope_game_fifo", O_RDONLY);      // open FIFO pipe for reading
    if (fifo_fd < 0) {
        perror("OpenGL FIFO open failed");                // print error if it fails
        exit(EXIT_FAILURE);                               // exit
    }
    glutMainLoop();                                       // start the main loop (never returns)
    close(fifo_fd);                                       // close FIFO after loop ends (not reached)
}