import pygame

class InputHandler:
    def get_actions(self):
        actions = {
            "QUIT": False, 
            "BACK": False, 
            "MOUSE_CLICK": None, 
            "MOUSE_POS": pygame.mouse.get_pos()
        }
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                actions["QUIT"] = True
            
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    actions["BACK"] = True
            
            if event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1: # Left click
                    actions["MOUSE_CLICK"] = pygame.mouse.get_pos()
                    
        return actions