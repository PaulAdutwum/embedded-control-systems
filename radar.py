import pygame
import serial
import math
import sys




WIDTH = 1200
HEIGHT = 700
# *** UPDATE THIS PORT NAME TO MATCH YOUR MAC TERMINAL RESULT ***
SERIAL_PORT = "/dev/cu.usbmodem2101" 
BAUD_RATE = 9600

# --- SETUP SERIAL ---
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
    print(f"Connected to {SERIAL_PORT}")
except:
    print(f"ERROR: Could not connect to {SERIAL_PORT}. Check connection.")
    ser = None

# --- SETUP PYGAME ---
pygame.init()
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Sentry Radar System")

# Colors
GREEN = (98, 245, 31)
RED = (255, 10, 10)
DARK_GREEN = (0, 50, 0)
BLACK = (0, 0, 0)
FADE_BLACK = (0, 0, 0, 10) # 10 is the transparency level for the trail

# Fonts
font_large = pygame.font.SysFont('Consolas', 30)
font_small = pygame.font.SysFont('Consolas', 20)
font_huge = pygame.font.SysFont('Consolas', 60, bold=True)

# Global Variables
iAngle = 0
iDistance = 0
running = True


fade_surface = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)
fade_surface.fill(FADE_BLACK)

def read_serial():
    global iAngle, iDistance
    if ser and ser.in_waiting > 0:
        try:
            data = ser.read_until(b'.').decode('utf-8').strip()
            data = data.replace('.', '')
            if ',' in data:
                parts = data.split(',')
                iAngle = int(parts[0])
                iDistance = int(parts[1])
        except:
            pass

def draw_grid():
    center_x = WIDTH // 2
    center_y = int(HEIGHT - HEIGHT * 0.074)
    # Draw faint green rings
    for r_scale in [0.25, 0.5, 0.75, 1.0]:
        radius = int((WIDTH/2) * r_scale)
        pygame.draw.circle(screen, DARK_GREEN, (center_x, center_y), radius, 1)
    # Draw faint angle lines
    for angle in [30, 60, 90, 120, 150]:
        rad = math.radians(angle)
        end_x = center_x + (WIDTH/2) * math.cos(rad)
        end_y = center_y - (WIDTH/2) * math.sin(rad)
        pygame.draw.line(screen, DARK_GREEN, (center_x, center_y), (end_x, end_y), 1)

def draw_radar_elements():
    center_x = WIDTH // 2
    center_y = int(HEIGHT - HEIGHT * 0.074)
    
    # Calculate Line Position
    rad = math.radians(iAngle)
    length = WIDTH / 2
    line_x = center_x + length * math.cos(rad)
    line_y = center_y - length * math.sin(rad)
    
    # 1. DRAW THE SCANNING LINE
    # If Sentry Locked (Distance < 20), make line RED, otherwise GREEN
    line_color = RED if (iDistance < 50 and iDistance > 0) else GREEN
    pygame.draw.line(screen, line_color, (center_x, center_y), (line_x, line_y), 2)

    # 2. DRAW THE OBJECT
    if iDistance < 100 and iDistance > 0:
        # Calculate Object Position
        pixsDistance = iDistance * ((HEIGHT - HEIGHT * 0.1666) * 0.025)
        obj_x = center_x + pixsDistance * math.cos(rad)
        obj_y = center_y - pixsDistance * math.sin(rad)
        
        # LOGIC: RED vs GREEN
        if iDistance < 50:
            
            pygame.draw.circle(screen, RED, (int(obj_x), int(obj_y)), 15)
            pygame.draw.line(screen, RED, (center_x, center_y), (obj_x, obj_y), 5)
            
            # Draw the Distance Number right next to the object
            text_tag = font_small.render(f"{iDistance}cm", True, (255, 255, 255))
            screen.blit(text_tag, (obj_x + 10, obj_y - 20))
            
        else:
           
            pygame.draw.circle(screen, GREEN, (int(obj_x), int(obj_y)), 10)

def draw_ui():
    # Show Angle and Range in corner
    text_angle = font_large.render(f"Angle: {iAngle}", True, GREEN)
    text_dist = font_large.render(f"Range: {iDistance}cm", True, GREEN)
    screen.blit(text_angle, (10, 10))
    screen.blit(text_dist, (10, 50))

    # Show BIG WARNING if locked
    if iDistance < 50 and iDistance > 0:
        text_alert = font_huge.render("TARGET LOCKED", True, RED)
        # Center the text
        text_rect = text_alert.get_rect(center=(WIDTH/2, HEIGHT/2 - 100))
        screen.blit(text_alert, text_rect)

# --- MAIN LOOP ---
clock = pygame.time.Clock()

while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    read_serial()
    
    # Draw Background Fade (Creates the motion blur trail)
    screen.blit(fade_surface, (0,0))
    
    draw_grid()
    draw_radar_elements()
    draw_ui()
    
    pygame.display.flip()
    clock.tick(60)

pygame.quit()
sys.exit()