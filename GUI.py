import pygame
from pygame.locals import *
import asyncio
import bleak
from bleak import BleakScanner

pygame.init()

width = 600
height = 600

screen = pygame.display.set_mode((width, height))
pygame.display.set_caption('Tractor GUI')
font = pygame.font.SysFont('Arial', 30)
font2 = pygame.font.SysFont('Arial', 60)

#Colors
white = (255, 255, 255)
black = (0, 0, 0)

clicked = False

class button():
    button_col = (25, 150, 225)
    hover_col = (75, 225, 255)
    click_col = (50, 150, 225)
    text_col = (0, 0, 0)
    width = 100
    height = 50

    def __init__(self, x, y, text):
        self.x = x
        self.y = y
        self.text = text

    def handle_Button(self):
        global clicked
        outcome = False

        mouse_pos = pygame.mouse.get_pos()

        button_Box = Rect(self.x, self.y, self.width, self.height)

        if(button_Box.collidepoint(mouse_pos)):
            if(pygame.mouse.get_pressed()[0] == True):
                clicked = True
                pygame.draw.rect(screen, self.click_col, button_Box)
            elif pygame.mouse.get_pressed()[0] == False and clicked == True:
                clicked = False
                outcome = True
            else:
                pygame.draw.rect(screen, self.hover_col, button_Box)
        else:
            pygame.draw.rect(screen, self.button_col, button_Box)
        text_render = font.render(self.text, True, self.text_col)
        text_length = text_render.get_width()

        screen.blit(text_render, (self.x + int(self.width / 2 - text_length / 2) , self.y + 5))

        return outcome

class trip_report():
    def __init__(self):
        self.counter = 0

    def print_background(self):
        trip_report_bg = Rect(150, 300, 300, 250)
        pygame.draw.rect(screen, (214,214,214), trip_report_bg)
        text_render = font.render('Trip Report', True, black)
        text_length = text_render.get_width()
        screen.blit(text_render, (150 + int(300 / 2 - text_length / 2) , 300 + 5))
        text_render = font2.render('Autonomous Tractor', True, black)
        text_length = text_render.get_width()
        screen.blit(text_render, (0 + int(600 / 2 - text_length / 2) , 60 + 5))
        

        


start = button(150, 200, 'Start')
stop = button(350, 200, 'Stop')
trip_report_screen = trip_report()

running = True



while running:
    screen.fill(white)

    if start.handle_Button():
        print('Start')
    elif stop.handle_Button():
        print('Stop')

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    trip_report_screen.print_background()

    pygame.display.update()

pygame.quit()
