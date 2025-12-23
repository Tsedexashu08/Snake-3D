#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glut.h>
#include <math.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm> 
#include <ctime>

using namespace std;


struct Segment {
    float x, z;  // Position on ground plane
};

struct Apple {
    float x, z;
    bool active;
};

// --- Game States ---
enum GameState { PLAYING, GAME_OVER };
GameState gameState = PLAYING;

// --- Game Constants ---
enum Direction { UP, DOWN, LEFT, RIGHT };
const float APPLE_SIZE = 0.6f;
const int MAX_APPLES = 3;

// --- Game Variables ---
Direction currentDir = UP;
vector<Segment> snake = {{0, 0}, {0, 1}, {0, 2}};  // Initial snake
vector<Apple> apples;  // Active apples
int score = 0;
int highScore = 0;

// --- Global Variables ---
float rotateY = 0.0f;
GLuint groundTexture = 0;
GLuint bgTexture = 0;
GLuint wallTexture = 0;
GLuint snakeTexture = 0;
GLuint snakeHeadTexture = 0;
GLuint appleTexture = 0;

// --- Texture Loading Function ---
GLuint loadTexture(const char *filename, bool flipVertically = true) {
    GLuint textureID = 0;
    stbi_set_flip_vertically_on_load(flipVertically);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);

    if (!data) {
        printf("Error: Could not load texture %s\n", filename);
        printf("STB Reason: %s\n", stbi_failure_reason());
        return 0;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Determine format (using old OpenGL constants)
    GLenum format = GL_RGB;

    if (nrChannels == 1) {
        format = GL_LUMINANCE; // Grayscale
    }
    else if (nrChannels == 2) {
        format = GL_LUMINANCE_ALPHA; // Grayscale + Alpha
    }
    else if (nrChannels == 3) {
        format = GL_RGB; // RGB
    }
    else if (nrChannels == 4) {
        format = GL_RGBA; // RGBA
    }
    else {
        printf("Error: Unsupported number of channels: %d\n", nrChannels);
        stbi_image_free(data);
        return 0;
    }

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
                 format, GL_UNSIGNED_BYTE, data);

    // Free image data
    stbi_image_free(data);

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    printf("Loaded texture: %s (%dx%d, %d channels)\n", filename, width, height, nrChannels);

    return textureID;
}

// --- Apple Functions ---
void spawnApple() {
    if (apples.size() >= MAX_APPLES) return;
    
    Apple newApple;
    
    // Find valid position (not on snake, not on walls)
    bool validPosition = false;
    int attempts = 0;
    
    while (!validPosition && attempts < 100) {
        // Random position within bounds (keep away from edges)
        newApple.x = (rand() % 16) - 8;  // -8 to 8
        newApple.z = (rand() % 16) - 8;  // -8 to 8
        
        // Round to grid (snake moves in 1 unit increments)
        newApple.x = floor(newApple.x) + 0.5f;
        newApple.z = floor(newApple.z) + 0.5f;
        
        // Check if position is valid
        validPosition = true;
        
        // Check collision with snake
        for (const auto& segment : snake) {
            float dx = segment.x - newApple.x;
            float dz = segment.z - newApple.z;
            float distance = sqrt(dx*dx + dz*dz);
            
            if (distance < 1.0f) {  // Too close to snake
                validPosition = false;
                break;
            }
        }
        
        // Check collision with existing apples
        for (const auto& apple : apples) {
            float dx = apple.x - newApple.x;
            float dz = apple.z - newApple.z;
            float distance = sqrt(dx*dx + dz*dz);
            
            if (distance < 2.0f) {  // Too close to another apple
                validPosition = false;
                break;
            }
        }
        
        // Check if inside walls (with margin)
        if (newApple.x <= -9.5f || newApple.x >= 9.5f ||
            newApple.z <= -9.5f || newApple.z >= 9.5f) {
            validPosition = false;
        }
        
        attempts++;
    }
    
    if (validPosition) {
        newApple.active = true;
        apples.push_back(newApple);
    }
}

void checkAppleCollision() {
    if (snake.empty() || gameState != PLAYING) return;
    
    Segment head = snake[0];
    
    for (auto it = apples.begin(); it != apples.end(); ) {
        if (!it->active) {
            it = apples.erase(it);
            continue;
        }
        
        float dx = head.x - it->x;
        float dz = head.z - it->z;
        float distance = sqrt(dx*dx + dz*dz);
        
        // If snake head touches apple
        if (distance < 0.8f) {
            // Increase score and update high score
            score++;
            if (score > highScore) highScore = score;
            
            // Grow snake by adding segment at tail
            Segment newSegment;
            if (snake.size() > 1) {
                // Extend in direction opposite to movement
                Segment tail = snake.back();
                Segment beforeTail = snake[snake.size() - 2];
                
                newSegment.x = tail.x + (tail.x - beforeTail.x);
                newSegment.z = tail.z + (tail.z - beforeTail.z);
            } else {
                // Just duplicate last segment
                newSegment = snake.back();
            }
            snake.push_back(newSegment);
            
            // Remove apple
            it = apples.erase(it);
            
            // Spawn new apple
            spawnApple();
            
            break;
        } else {
            ++it;
        }
    }
}

void initApples() {
    apples.clear();
    spawnApple();
}

// --- Collision Detection ---
bool checkWallCollision() {
    if (snake.empty()) return false;
    
    Segment head = snake[0];
    // Check if head hits boundary walls
    return (head.x <= -10 || head.x >= 10 || head.z <= -10 || head.z >= 10);
}

bool checkSelfCollision() {
    if (snake.empty()) return false;
    
    Segment head = snake[0];
    
    for (size_t i = 1; i < snake.size(); i++) {
        if (head.x == snake[i].x && head.z == snake[i].z) {
            return true;
        }
    }
    return false;
}

// --- Drawing Functions ---
void drawTexturedWall(float x, float z, float w, float d, float h) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, wallTexture);
    glColor3f(1.0f, 1.0f, 1.0f);

    glPushMatrix();
    glTranslatef(x, h/2.0f, z);
    glScalef(w, h, d);

    float hw = 0.5f; 
    glBegin(GL_QUADS);
        // Front face
        glNormal3f(0, 0, 1);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-hw, -hw,  hw);
        glTexCoord2f(2.0f, 0.0f); glVertex3f( hw, -hw,  hw);
        glTexCoord2f(2.0f, 2.0f); glVertex3f( hw,  hw,  hw);
        glTexCoord2f(0.0f, 2.0f); glVertex3f(-hw,  hw,  hw);
        
        // Back face
        glNormal3f(0, 0, -1);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-hw, -hw, -hw);
        glTexCoord2f(0.0f, 2.0f); glVertex3f(-hw,  hw, -hw);
        glTexCoord2f(2.0f, 2.0f); glVertex3f( hw,  hw, -hw);
        glTexCoord2f(2.0f, 0.0f); glVertex3f( hw, -hw, -hw);

        // Top face
        glNormal3f(0, 1, 0);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-hw,  hw, -hw);
        glTexCoord2f(2.0f, 0.0f); glVertex3f(-hw,  hw,  hw);
        glTexCoord2f(2.0f, 2.0f); glVertex3f( hw,  hw,  hw);
        glTexCoord2f(0.0f, 2.0f); glVertex3f( hw,  hw, -hw);

        // Right face
        glNormal3f(1, 0, 0);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( hw, -hw, -hw);
        glTexCoord2f(2.0f, 0.0f); glVertex3f( hw,  hw, -hw);
        glTexCoord2f(2.0f, 2.0f); glVertex3f( hw,  hw,  hw);
        glTexCoord2f(0.0f, 2.0f); glVertex3f( hw, -hw,  hw);

        // Left face
        glNormal3f(-1, 0, 0);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-hw, -hw, -hw);
        glTexCoord2f(2.0f, 0.0f); glVertex3f(-hw, -hw,  hw);
        glTexCoord2f(2.0f, 2.0f); glVertex3f(-hw,  hw,  hw);
        glTexCoord2f(0.0f, 2.0f); glVertex3f(-hw,  hw, -hw);
    glEnd();

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

void drawBackground() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, bgTexture);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void drawTexturedCube(float size) {
    float s = size / 2.0f;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, snakeTexture);

    glBegin(GL_QUADS);
        // Front Face
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s, -s, s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(s, -s, s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(s, s, s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s, s, s);

        // Back Face
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s, s, -s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(s, s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(s, -s, -s);

        // Top Face
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s, s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s, s, s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(s, s, s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(s, s, -s);

        // Bottom Face
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(s, -s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(s, -s, s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s, -s, s);

        // Right Face
        glTexCoord2f(1.0f, 0.0f); glVertex3f(s, -s, -s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(s, s, -s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(s, s, s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(s, -s, s);

        // Left Face
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s, -s, s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s, s, s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s, s, -s);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void drawApple(float x, float z) {
    glPushMatrix();
    glTranslatef(x, APPLE_SIZE/2.0f, z); // Center apple
    
    if (appleTexture) {
        // Textured apple
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, appleTexture);
        
        // Draw apple as a sphere
        GLUquadric* quad = gluNewQuadric();
        gluQuadricTexture(quad, GL_TRUE);
        gluQuadricNormals(quad, GLU_SMOOTH);
        
        // Rotate for better texture mapping
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluSphere(quad, APPLE_SIZE/2.0f, 16, 16);
        
        gluDeleteQuadric(quad);
        glDisable(GL_TEXTURE_2D);
    } else {
        // Colored apple (fallback)
        glDisable(GL_TEXTURE_2D);
        
        // Main apple body (red)
        glColor3f(0.8f, 0.1f, 0.1f);
        
        // Draw as sphere
        glPushMatrix();
        glutSolidSphere(APPLE_SIZE/2.0f, 16, 16);
        glPopMatrix();
        
        // Draw stem
        glColor3f(0.4f, 0.2f, 0.1f);
        glLineWidth(3.0f);
        glBegin(GL_LINES);
            glVertex3f(0.0f, APPLE_SIZE/2.0f, 0.0f);
            glVertex3f(0.0f, APPLE_SIZE/2.0f + 0.2f, 0.0f);
        glEnd();
        
        // Draw leaf
        glColor3f(0.2f, 0.6f, 0.2f);
        glPushMatrix();
        glTranslatef(0.1f, APPLE_SIZE/2.0f + 0.1f, 0.0f);
        glRotatef(45.0f, 0.0f, 0.0f, 1.0f);
        glScalef(1.0f, 0.1f, 0.5f); // Flatten
        glutSolidSphere(0.1f, 8, 8);
        glPopMatrix();
        
        glEnable(GL_TEXTURE_2D);
    }
    
    glPopMatrix();
}

void drawSnakeHead(Direction facing) {
    float headSize = 0.9f;  // Slightly larger than body
    float s = headSize / 2.0f;
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, snakeHeadTexture);  // Head texture
    
    // Rotate head based on facing direction
    switch (facing) {
        case UP:    glRotatef(180.0f, 0.0f, 1.0f, 0.0f); break;   // Looking up (-Z)
        case DOWN:  glRotatef(0.0f, 0.0f, 1.0f, 0.0f); break;     // Looking down (+Z)
        case LEFT:  glRotatef(90.0f, 0.0f, 1.0f, 0.0f); break;    // Looking left (-X)
        case RIGHT: glRotatef(-90.0f, 0.0f, 1.0f, 0.0f); break;   // Looking right (+X)
    }
    
    // Draw head cube with special texture mapping for face
    glBegin(GL_QUADS);
        // FRONT FACE (where the eyes/mouth go)
        glTexCoord2f(0.25f, 0.25f); glVertex3f(-s, -s,  s);
        glTexCoord2f(0.75f, 0.25f); glVertex3f( s, -s,  s);
        glTexCoord2f(0.75f, 0.75f); glVertex3f( s,  s,  s);
        glTexCoord2f(0.25f, 0.75f); glVertex3f(-s,  s,  s);
        
        // BACK FACE (back of head)
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s,  s, -s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( s,  s, -s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( s, -s, -s);
        
        // TOP FACE (scalp)
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s,  s, -s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s,  s,  s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( s,  s,  s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( s,  s, -s);
        
        // BOTTOM FACE (chin/neck area)
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( s, -s, -s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( s, -s,  s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s, -s,  s);
        
        // RIGHT SIDE FACE (cheek)
        glTexCoord2f(0.0f, 0.0f); glVertex3f( s, -s, -s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( s,  s, -s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( s,  s,  s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( s, -s,  s);
        
        // LEFT SIDE FACE (cheek)
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s, -s,  s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s,  s,  s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s,  s, -s);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
}

void drawSnakeSegment(float x, float z, bool isHead, Direction facing) {
    glPushMatrix();
    glTranslatef(x, 0.5f, z);
    
    if (isHead) {
        // For the head, use the head texture
        if (snakeHeadTexture) {
            drawSnakeHead(facing);
        } else {
            // Fallback to sphere if no head texture
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, snakeTexture);
            GLUquadric* quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); // Rotate so "pole" is up
            gluSphere(quad, 0.6, 16, 16);
            gluDeleteQuadric(quad);
            glDisable(GL_TEXTURE_2D);
        }
    } else {
        drawTexturedCube(0.8f);
    }
    
    glPopMatrix();
}

void drawScene() {
    // Ground
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, groundTexture);
    glNormal3f(0, 1, 0);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-10.0f, 0.0f, -10.0f);
        glTexCoord2f(5.0f, 0.0f); glVertex3f( 10.0f, 0.0f, -10.0f);
        glTexCoord2f(5.0f, 5.0f); glVertex3f( 10.0f, 0.0f,  10.0f);
        glTexCoord2f(0.0f, 5.0f); glVertex3f(-10.0f, 0.0f,  10.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Walls
    drawTexturedWall(0, -10, 20.5, 0.5, 1.5);  // South wall
    drawTexturedWall(0, 10, 20.5, 0.5, 1.5);   // North wall
    drawTexturedWall(-10, 0, 0.5, 20.5, 1.5);  // West wall
    drawTexturedWall(10, 0, 0.5, 20.5, 1.5);   // East wall
    
    // Interior walls (obstacles)
    drawTexturedWall(-4, -4, 4, 0.8, 1.2);
    drawTexturedWall(5, 3, 0.8, 6, 1.2);

    // Draw apples
    glDisable(GL_LIGHTING);
    for (const auto& apple : apples) {
        if (apple.active) {
            drawApple(apple.x, apple.z);
        }
    }
    
    // Snake
    for (size_t i = 0; i < snake.size(); i++) {
        if (i == 0) {
            drawSnakeSegment(snake[i].x, snake[i].z, true, currentDir);
        } else {
            drawSnakeSegment(snake[i].x, snake[i].z, false, currentDir);
        }
    }
    glEnable(GL_LIGHTING);
}

void drawGameOverScreen() {
    // Dark overlay
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Semi-transparent black overlay
    glColor4f(0, 0, 0, 0.7);
    glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(800, 0);
        glVertex2f(800, 600);
        glVertex2f(0, 600);
    glEnd();
    
    // Game Over text
    glColor3f(1, 0, 0);  // Red
    glRasterPos2f(350, 400);
    char gameOverText[] = "GAME OVER";
    for (char* c = gameOverText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    }
    
    // Score
    glColor3f(1, 1, 1);  // White
    glRasterPos2f(350, 350);
    char scoreText[50];
    sprintf(scoreText, "Score: %d", score);
    for (char* c = scoreText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    
    // High Score
    glRasterPos2f(350, 320);
    char highScoreText[50];
    sprintf(highScoreText, "High Score: %d", highScore);
    for (char* c = highScoreText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    
    // Restart instructions
    glColor3f(0.8, 0.8, 0);  // Yellow
    glRasterPos2f(300, 280);
    char restartText[] = "Press SPACE to restart";
    for (char* c = restartText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void drawHUD() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Score display (top-left)
    glColor3f(1, 1, 1);
    glRasterPos2f(10, 580);
    char scoreText[50];
    sprintf(scoreText, "Score: %d", score);
    for (char* c = scoreText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    
    // High score display (top-right)
    glRasterPos2f(650, 580);
    char highScoreText[50];
    sprintf(highScoreText, "High Score: %d", highScore);
    for (char* c = highScoreText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

// --- Game Logic Functions ---
void moveSnake() {
    if (gameState != PLAYING) return;
    
    // Calculate new head position based on direction
    Segment newHead = snake[0];
    switch (currentDir) {
        case UP:    newHead.z -= 1.0f; break;
        case DOWN:  newHead.z += 1.0f; break;
        case LEFT:  newHead.x -= 1.0f; break;
        case RIGHT: newHead.x += 1.0f; break;
    }
    
    // Insert new head at front
    snake.insert(snake.begin(), newHead);
    // Remove tail (unless we just ate an apple - handled in checkAppleCollision)
    snake.pop_back();
}

void checkGameOver() {
    if (checkWallCollision() || checkSelfCollision()) {
        gameState = GAME_OVER;
        printf("Game Over! Final Score: %d | High Score: %d\n", score, highScore);
    }
}

void resetGame() {
    // Reset snake to initial state
    snake = {{0, 0}, {0, 1}, {0, 2}};
    currentDir = UP;
    
    // Clear apples and spawn new ones
    apples.clear();
    initApples();
    
    // Reset score (keep high score)
    score = 0;
    
    // Return to playing state
    gameState = PLAYING;
    
    printf("Game Restarted!\n");
}

// --- GLUT Callbacks ---
void update(int value) {
    if (gameState == PLAYING) {
        moveSnake();
        checkAppleCollision();
        checkGameOver();
    }
    
    glutPostRedisplay();
    glutTimerFunc(150, update, 0);  // Update every 150ms
}

void keyboard(unsigned char key, int x, int y) {
    if (key == ' ' && gameState == GAME_OVER) {
        resetGame();
    }
}

void specialKeys(int key, int x, int y) {
    if (gameState != PLAYING) return;  // Only accept input when playing
    
    switch (key) {
        case GLUT_KEY_UP:
            if (currentDir != DOWN) currentDir = UP;
            break;
        case GLUT_KEY_DOWN:
            if (currentDir != UP) currentDir = DOWN;
            break;
        case GLUT_KEY_LEFT:
            if (currentDir != RIGHT) currentDir = LEFT;
            break;
        case GLUT_KEY_RIGHT:
            if (currentDir != LEFT) currentDir = RIGHT;
            break;
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Draw background (skybox)
    drawBackground();
    
    // Set up 3D camera
    glLoadIdentity();
    gluLookAt(0.0, 18.0, 22.0,  // Camera position
              0.0, 0.0, 0.0,    // Look at point
              0.0, 1.0, 0.0);   // Up vector
    
    // Set lighting
    GLfloat lightPos[] = {10.0f, 20.0f, 10.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    
    // Draw 3D scene
    drawScene();
    
    // Draw HUD (score display)
    drawHUD();
    
    // Draw game over screen if needed
    if (gameState == GAME_OVER) {
        drawGameOverScreen();
    }
    
    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w/h, 1.0, 200.0);
    glMatrixMode(GL_MODELVIEW);
}

void init() {
    // Initialize random seed
    srand(static_cast<unsigned>(time(0)));
    
    // Set up OpenGL
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    
    // Set up lighting
    GLfloat globalAmbient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    GLfloat ambientLight[] = {0.4f, 0.4f, 0.4f, 1.0f};
    GLfloat diffuseLight[] = {0.7f, 0.7f, 0.7f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    
    // Load textures
    groundTexture = loadTexture("textures/grass_1.bmp", true);
    bgTexture = loadTexture("textures/bg4.png", true);
    wallTexture = loadTexture("textures/b2.bmp", true);
    snakeTexture = loadTexture("textures/s2.bmp", true);
    // snakeHeadTexture = loadTexture("textures/snake_head.png", true); 
    
    // Load apple texture
    appleTexture = loadTexture("textures/apple.png", true);
    if (!appleTexture) {
        printf("Apple texture not found. Using colored apple.\n");
    }

    // Check if textures loaded successfully
    if (!groundTexture || !bgTexture || !wallTexture || !snakeTexture)
    {
        printf("Warning: Some textures failed to load!\n");
        printf("Make sure your image files are in the correct directory.\n");
        printf("Supported formats: JPG, PNG, BMP, TGA, PSD, GIF, HDR, PIC, PNM\n");
    }
    
    // Initialize game objects
    initApples();
    
    printf("=== 3D Snake Game ===\n");
    printf("Controls: Arrow Keys to move\n");
    printf("Goal: Eat apples to grow and increase score\n");
    printf("Avoid: Walls and your own tail\n");
    printf("Game Over: Press SPACE to restart\n");
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("3D Snake Game - Eat Apples!");
    
    init();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(150, update, 0);
    
    glutMainLoop();
    return 0;
}