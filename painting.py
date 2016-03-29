from __future__ import division 
#pythons future module to make bspline division work
import pygame, scipy, numpy, random, copy
from pygame.locals import *
from scipy import signal, ndimage
from pycurve import Bspline 
#external library from http://pygame.org/project-pycurve-1390-.html
#allows one to create a spline curve from a set of control points
import inputbox
from Tkinter import *
from tkFileDialog import *


#The Thesis paper that outlined the algorithim structrue can be found below:
#http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.295.9865&rep=rep1
#&type=pdf

##### general functions ########

def grayscale(array):
    """converts a rgb array into a 2d array with the gray values"""
    #note that the grayscale here calculated via luminance formula
    r = array[:, :, 0]
    g = array[:, :, 1]
    b = array[:, :, 2]
    #slice the arrays to increase speed instead of looping through each cell
    rFactor = 0.298
    gFactor = 0.587
    bFactor = 0.114
    #multiply each minimatrix by the factor
    mr = numpy.multiply(rFactor, r)
    mg = numpy.multiply(gFactor, g)
    mb = numpy.multiply(bFactor, b)
    #call add twice to add 3 things
    gray = numpy.add(numpy.add(mr,mg), mb)
    
    return gray #2d single valued array
    
#gaussian blur in pygame adapted from
#https://code.google.com/p/pycam/source/
#browse/trunk/pycam/examples/blurFrame.py?r=92

def gaussianBlur(imageArray,sigma):
    """This function takes a pygame surface, converts it to a numpy array
    carries out gaussian blur, converts back then returns the pygame surface.
    """
    #modified to take in sigma, converts to an array first
    np_array = pygame.surfarray.array3d(imageArray)
    #apply sciy's filter
    result = scipy.ndimage.filters.gaussian_filter(np_array, 
             sigma = (sigma, sigma, 0), order = 0, mode = 'reflect')
    # Convert back to a surface.          
    surface = pygame.surfarray.make_surface(result)
    return surface
    
def colorDistance(pixel1,pixel2):
    """This function takes 2 pixels and compares their rgb values. it returns
    their vector distance in RGB space as a floating point value"""
    red   = pixel1[0] - pixel2[0]
    green = pixel1[1] - pixel2[1]
    blue  = pixel1[2] - pixel2[2]
    distance = float(((red)**2+(green)**2+(blue)**2)**0.5)
    return distance # note this returns a positive float

def summedArea(array):
    """This function takes an array and converts into a summedArea array"""
    mid = numpy.cumsum(array, 1) #making use of numpy's cumulative sum
    return numpy.cumsum(mid, 0)

def summedSquare(sumArea,x,y,brushSize):
    """This function takes an summed area table and computes the summed area
    in a square centered on xy with 'radius' brushSize"""
    pixels = 799
    tl = (x-brushSize,y-brushSize)
    tr = (x+brushSize,y-brushSize)
    bl = (x-brushSize,y+brushSize)
    br = (x+brushSize,y+brushSize)
    #general case
    if (x-brushSize >= 0 and x+brushSize <= pixels and 
            y-brushSize >= 0 and y+brushSize <= pixels):
        toAdd      = numpy.add(sumArea[tl[0]][tl[1]], sumArea[br[0]][br[1]])
        toSubtract = numpy.add(sumArea[tr[0]][tr[1]], sumArea[bl[0]][bl[1]])
        area       = numpy.subtract(toAdd, toSubtract)
    #account for sides    
    elif x-brushSize < 0 and y-brushSize >= 0 and y+brushSize <= pixels:
        area = numpy.subtract(sumArea[br[0]][br[1]], sumArea[tr[0]][tr[1]])
        
    elif x+brushSize > pixels and y-brushSize >= 0 and y+brushSize <= pixels:
        toAdd      = numpy.add(sumArea[tl[0]][tl[1]], sumArea[pixels][br[1]])
        toSubtract = numpy.add(sumArea[pixels][tr[1]],sumArea[bl[0]][bl[1]])
        area       = numpy.subtract(toAdd, toSubtract)
        
    elif y-brushSize < 0 and x-brushSize >= 0 and x+brushSize <= pixels:
        area = numpy.subtract(sumArea[br[0]][br[1]], sumArea[bl[0]][bl[1]])
        
    elif y+brushSize > pixels and x-brushSize >= 0 and x+brushSize <= pixels:
        toAdd      = numpy.add(sumArea[tl[0]][tl[1]], sumArea[br[0]][pixels])
        toSubtract = numpy.add(sumArea[tr[0]][tr[1]], sumArea[bl[0]][pixels])
        area       = numpy.subtract(toAdd, toSubtract)
    #account for corners
    elif x-brushSize < 0 and y-brushSize < 0:
        area = sumArea[br[0]][br[1]]
        
    elif x-brushSize < 0 and y+brushSize > pixels:
        area = numpy.subtract(sumArea[br[0]][pixels], sumArea[tr[0]][tr[1]])
        
    elif y-brushSize < 0 and x+brushSize > pixels:
        area = numpy.subtract(sumArea[pixels][br[1]], sumArea[bl[0]][bl[1]])
        
    elif y+brushSize > pixels and x+brushSize > pixels:
        toAdd      = numpy.add(sumArea[tl[0]][tl[1]], sumArea[pixels][pixels])
        toSubtract = numpy.add(sumArea[pixels][tr[1]], sumArea[bl[0]][pixels])
        area       = numpy.subtract(toAdd, toSubtract)
        
    return area

########## Animation Class ###########

class producePainting(object):
    
    def checkForEvent(self): #checks for general events
        for event in pygame.event.get():
            if event.type == QUIT:
                self.animation.quit = True
                return False
            elif event.type == KEYDOWN:
                if event.key == K_ESCAPE:
                    self.animation.loadingScreen = False
                    self.animation.myMenu.active = True
                    return False
                elif event.key == K_s:
                    if self.done == False:
                        self.skipToEnd = True
                        self.done      = True 
                elif event.key == K_q:
                    if self.speed > 5:
                        self.speed -= 5
                    elif self.speed > 1:
                        self.speed -=1
                elif event.key == K_w:
                    if self.speed < 5:
                        self.speed += 1
                    elif self.speed < 100:
                        self.speed += 5
                    
            elif event.type == MOUSEBUTTONDOWN and self.done==True:
                (eventX,eventY) = event.pos
                self.checkForSave(eventX, eventY)                                               
        return True
        
    def checkForSave(self, eventX, eventY):
        left   = 900
        right  = 1100
        top    = 550
        bottom = 650
        if (eventX > left and eventX < right 
            and eventY > top and eventY < bottom):
            root = Tk()
            fileName = asksaveasfilename(initialdir="C:\\Users\\Brent\\Desktop\\TermProject",filetypes=[("PNG", "*.png")],defaultextension=".png")
            root.destroy()
            root.mainloop()
            try:
                pygame.image.save(self.canvas,fileName)
                self.errorMessage = False
            except:
                self.errorMessage = True
            
        
    def drawAll(self):
        self.screen.fill(self.black)
        self.drawSourceImage()
        if self.skipToEnd:
            position = (0,0)
            self.canvas.blit(self.finalPaintingSurface, position) 
            self.screen.blit(self.canvas, position)       
        else:
            self.drawCanvas()            
            if (self.brushIndex < len(self.brushSizes) and self.brushCounter < len(self.brushStrokes[self.brushIndex])-1): 
                self.drawCurrentColor()
                self.drawBrushSize()
                self.drawSkipInstructions()
        if self.errorMessage:
            self.drawErrorMessage()
        if self.done:
            self.drawSaveButton()
        self.drawFrame()
        pygame.display.flip()
    
    def drawSkipInstructions(self):
        font         = pygame.font.SysFont('harrington', 36)
        color        = self.white
        antialias    = 1
        text         = "Press 'S' to skip to End"
        pos          = (1000, 700)
        textSurface  = font.render(text, antialias,color)
        position     = textSurface.get_rect(center = pos)
        self.screen.blit(textSurface,position)
        
        text         = "Press 'Q' to slow down "
        pos          = (1000, 735)
        textSurface  = font.render(text, antialias,color)
        position     = textSurface.get_rect(center = pos)
        self.screen.blit(textSurface,position)
        
        text         = "Press 'W' to speed up   "
        pos          = (1000, 770)
        textSurface  = font.render(text, antialias,color)
        position     = textSurface.get_rect(center = pos)
        self.screen.blit(textSurface,position)
        
        font2         = pygame.font.SysFont('gabriola', 36)
        text         = "speed = %d"%(self.speed)
        pos          = (1000, 650)
        textSurface  = font2.render(text, antialias,color)
        position     = textSurface.get_rect(center = pos)
        self.screen.blit(textSurface,position)
        
    def drawErrorMessage(self):
        font = pygame.font.SysFont('harrington', 20)
        color = self.red
        antialias = 1
        text = "File not saved, Please Try Again"
        pos = (1000, 750)
        textSurface = font.render(text, antialias,color)
        position = textSurface.get_rect(center = pos)
        self.screen.blit(textSurface,position)
            
    def drawFrame(self):
        bottom = (0,  778)
        right  = (778,  0)
        self.screen.blit(self.frameBottom, bottom)
        self.screen.blit(self.frameLeft,   (0,0))
        self.screen.blit(self.frameTop,    (0,0))
        self.screen.blit(self.frameRight,  right)
        
    
    def drawSaveButton(self):
        self.screen.blit(self.button, (860, 560)) 
        font        = pygame.font.SysFont('harrington', 36)
        color       = self.black
        antialias   = 1
        text        = "Save Image"
        pos         = (1000, 600)
        textSurface = font.render(text, antialias,color)
        position    = textSurface.get_rect(center = pos)
        self.screen.blit(textSurface,position)
        
        
    def drawCurrentColor(self):
        (tlx,tly)   = (850,450)
        width       = height = 100
        color = self.brushStrokes[self.brushIndex][self.brushCounter][1]
        pygame.draw.rect(self.screen, color,[tlx,tly, width, height])
        font        = pygame.font.SysFont('gabriola', 36)
        color       = self.white
        antialias   = 1
        text        = "Color"
        pos         = (900, 600)
        textSurface = font.render(text, antialias,color)
        position    = textSurface.get_rect(center = pos)
        self.screen.blit(textSurface,position)
        
        
    def drawBrushSize(self):
        center    = (1100,500)
        brushSize = self.brushStrokes[self.brushIndex][self.brushCounter][2]
        pygame.draw.circle(self.screen, self.white, center,brushSize)
        font      = pygame.font.SysFont('gabriola', 36)
        color     = self.white
        antialias = 1
        text      = "Brush Size"
        text2     = "%d"%(brushSize)
        pos       = (1100, 600)
        if brushSize < 10: pos2 = (1152, 550)
        else:              pos2 = (1145, 550)
        textSurface  = font.render(text,  antialias,color)
        textSurface2 = font.render(text2, antialias,color)
        position     = textSurface.get_rect(center = pos)
        position2    = textSurface.get_rect(center = pos2)
        self.screen.blit(textSurface,  position)
        self.screen.blit(textSurface2, position2)
             
    def drawCanvas(self):
        self.screen.blit(self.canvas, (0,0))
        index = self.brushIndex
        counter = self.brushCounter
        if self.brushIndex < len(self.brushSizes) and (counter
                           < len(self.brushStrokes[index])-1):
            controlPoints = self.brushStrokes[index][counter][0]
            color = self.brushStrokes[index][counter][1]
            brushSize = self.brushStrokes[index][counter][2]
            minLength = 3
            if self.bSpline and len(controlPoints) > minLength: 
                #bspline part below adapted from the imported module example                  
                n = len(controlPoints) - 1 # n = len(P) - 1; (P[0], ... P[n])
                k = 3 #degree of curve                     
                m = n + k + 1   # property of b-splines: m = n + k + 1
                _t = 1 / (m - k * 2)
                t = k * [0] + [t_ * _t for t_ in xrange(m-(k * 2)+1)] + [1]*k
                S = Bspline(controlPoints, t, k)
                step_size = 1 / (n*10)
                for i in xrange(n*10):
                    t_ = i * step_size
                    try: x, y = S(t_) 
                    #if curve not defined here (t_ is out of domain): skip
                    except: continue
                    x, y = int(x), int(y)
                    point = (x,y)
                    pygame.draw.circle(self.canvas, color, point, brushSize)
            else:
                for point in controlPoints:
                    pygame.draw.circle(self.canvas, color, point, brushSize)
              
    def drawSourceImage(self):
        image          = self.sourceImg.convert()
        center         = (360, 360)
        scaledImg      = pygame.transform.scale(image, center)
        screenPosition = (820,20)
        self.screen.blit(scaledImg, screenPosition)
        font        = pygame.font.SysFont('gabriola', 36)
        color       = self.white
        antialias   = 1
        text        = "Original Image"
        pos         = (1000, 400)
        textSurface = font.render(text, antialias,color)
        position    = textSurface.get_rect(center = pos)
        self.screen.blit(textSurface,position)
        
        
    def createPainting(self):
        """calculates all the necssary brushstrokes"""
        #convert the surface into a 3-d array arrray (x,y,RGB)
        pCanvasSurface = pygame.Surface.copy(self.canvas).convert()
        for index in xrange(len(self.brushSizes)):#loop through each brush
            brushSize  = self.brushSizes[index]
            pCanvas2D  = pygame.surfarray.pixels2d(pCanvasSurface)
            pCanvas    = pygame.surfarray.pixels3d(pCanvasSurface)
            #create a blurred reference image based on brushsize and f-sigma
            referenceImage = gaussianBlur(self.sourceImg, 
                                            brushSize*self.blurFactor)
            referenceImage2D  = pygame.surfarray.pixels2d(referenceImage)
            referenceImage    = pygame.surfarray.pixels3d(referenceImage)
            #create summedArea tables of the RGB values of ref img and canvas
            summedAreaCanvas2D    = summedArea(pCanvas2D)
            summedAreaReference2D = summedArea(referenceImage2D)
            summedAreaCanvas      = summedArea(pCanvas)
            summedAreaReference   = summedArea(referenceImage)
            #create grayscale arrays
            grayImage   = grayscale(referenceImage)
            sobelImageX = scipy.ndimage.filters.sobel(grayImage,0)
            sobelImageY = scipy.ndimage.filters.sobel(grayImage,1)
            
            (width,height) = self.canvas.get_width(),self.canvas.get_height()
            for x in xrange(0,width+brushSize,brushSize): #extra last step
                for y in xrange(0,height+brushSize,brushSize):
                    #account for last step
                    if x > 799: x = 799
                    if y >799: y = 799
                    #caculate the color difference in the area
                    area1 = summedSquare(summedAreaCanvas,x,y,brushSize)
                    area2 = summedSquare(summedAreaReference,x,y,brushSize)
                    error = colorDistance(area1,area2)
                    if error > self.threshold:
                        if index!=0:
                            (i,j) = self.findMaxError(x,y,brushSize,
                                        referenceImage,pCanvas,
                                        summedAreaCanvas2D,summedAreaReference2D)
                        else:
                            (i,j) = (x,y)

                        pixel = referenceImage[i][j]
                        color = (pixel[0],pixel[1],pixel[2])                       
                        if self.maxStrokeLength <= 1:
                            controlPoints = [(i,j)]
                        else:
                            controlPoints = self.controlPoints(i,j,brushSize,
                                                referenceImage,pCanvas,color,
                                                sobelImageX,sobelImageY,
                                                pCanvasSurface)
                                                
                        for controlPoint in controlPoints:
                            pygame.draw.circle(pCanvasSurface, color, 
                                                controlPoint, brushSize)
                        self.brushStrokes[index].append((controlPoints,
                                                        color,brushSize))
        
        self.finalPaintingSurface = pCanvasSurface
                    
    def controlPoints(self,x,y,brushSize,referenceImage,pCanvas,
                        color,sobelImageX,sobelImageY,pCanvasSurface):
        controlPoints = [] 
        (pdx,pdy)     = (0,0)
        controlPoints.append((x,y)) #add the starting point
        for i in xrange(1, self.maxStrokeLength):
            #compute the image derivatives
            (px,py) = controlPoints[i-1]
            (gx,gy) = (sobelImageX[px][py]/16.0, sobelImageY[px][py]/16.0)
            g = ((gx)**2 + (gy)**2)**0.5
            #detect vanishing gradient
            if brushSize*g >= 1:
                #rotate gradient by 90 dgrees
                (dx,dy) = (-gy,gx)
                #if necessary reverse direction
                if i > 1 and ((pdx*dx) + (pdy*dy)) < 0:
                    dx *= -1
                    dy *= -1
            #else continue the stroke direction
            else:
                if i > 1:
                    (dx,dy) = (pdx,pdy)
                else:
                    return controlPoints
            #find the new point by adding the changes
            d = ((dx)**2 + (dy)**2)**0.5
            factor = brushSize/d
            (newX,newY) = numpy.add((px,py),(int(factor*dx),int(factor*dy)))
            if newX > 799 or newX < 0 or newY < 0 or newY > 799:
                return controlPoints
            ref = referenceImage[newX][newY]
            d1 = self.pixelDifference(ref,pCanvas[newX][newY])
            d2 = self.pixelDifference(ref,color)
            if i > self.minStrokeLength and d1 < d2:
                return controlPoints
            #append new point to k
            controlPoints.append((newX,newY))
            (pdx,pdy) = (dx,dy)
        return controlPoints#returns the list of control points       
        
    def findMaxError(self, x, y, brushSize,refImg ,pCanvas,
                     summedAreaCanvas2D, summedAreaReference2D):
        b      = brushSize
        maxPix = 800 - b
        (i,j)  = (x,y)
        if x > b and x < maxPix and y > b and y < maxPix:
            a1 = summedAreaCanvas2D[y-b:y+b+1, x-b:x+b+1]
            a2 = summedAreaReference2D[y-b:y+b+1, x-b:x+b+1]
            diffArea = numpy.subtract(a1, a2)
            diffArea = numpy.absolute(diffArea)
            (i,j) = numpy.unravel_index(numpy.argmax(diffArea), diffArea.shape)
            i += x - b
            j += y - b        
        return (i,j) 
                   
    def pixelDifference(self, list1,list2):
        result = numpy.subtract(list1,list2)
        difference = (result[0]**2 + result[1]**2 + result[2]**2)**0.5
        return difference           
    
    def initFrame(self):
        self.frameTop = (pygame.image.load(
        "C:\\Users\\Brent\\Desktop\\TermProject\\frameTop.png").convert())
        self.frameRight= (pygame.image.load(
        "C:\\Users\\Brent\\Desktop\\TermProject\\frameRight.png").convert())
        self.frameBottom = (pygame.image.load(
        "C:\\Users\\Brent\\Desktop\\TermProject\\frameBottom.png").convert())
        self.frameLeft = (pygame.image.load(
        "C:\\Users\\Brent\\Desktop\\TermProject\\frameLeft.png").convert())
    
    def initColors(self):
        self.black = (0,0,0)
        self.white = (255,255,255)
        self.red   = (255,0,0)                 

    def initCanvas(self):
        width = height = 800
        self.canvas = pygame.Surface((width,height)).convert()
        self.canvas.fill(self.white)
    
    def initBrushes(self):     
        self.brushStrokes = [[] for brushStroke in self.brushSizes]
        self.brushCounter = 0
        self.brushIndex   = 0
                            
    def init(self):
        pygame.mixer.init()
        self.music = pygame.mixer.Sound('C:\\Users\\Brent\\Desktop\\TermProject\\violin.wav')
        self.music.play(loops=-1)
        pygame.font.init()
        self.clock = pygame.time.Clock() #create a pygame clock to track time
        self.initFrame()
        self.initColors()
        self.initCanvas()#create canvas
        self.initBrushes()#create brushes, get all the brushStrokes
        self.createPainting()
        
        #sort the brushstrokes for rendering order
        for brushSize in self.brushStrokes:
            brushSize.sort(key=lambda x: x[1],reverse = True)
        
        self.skipToEnd    = False
        self.done         = False
        self.errorMessage = False
        self.speed = 100
        self.button = pygame.image.load('C:\\Users\\Brent\\Desktop\\TermProject\\button.png').convert()
        
    def updateBrushStroke(self):
        if self.brushIndex < len(self.brushSizes):
            if (self.brushCounter < 
                (len(self.brushStrokes[self.brushIndex]) -1)):
                self.brushCounter += 1
            else:
                self.brushIndex += 1
                self.brushCounter = 0
                
        else: 
            self.done = True  
               
    def run(self,screen,animation,sourceImg,
            minStrokeLength,maxStrokeLength,threshold,
            listOfBrushes,bSpline,blurFactor):
        #variables
        self.bSpline    = bSpline
        self.blurFactor = blurFactor
        self.minStrokeLength = minStrokeLength
        self.maxStrokeLength = maxStrokeLength
        self.threshold  = threshold
        self.brushSizes = listOfBrushes
        self.brushSizes.sort(reverse = True)
        
        self.screen    = screen
        self.animation = animation
        (sx,sy) = (800,800)
        self.sourceImg = pygame.transform.scale(sourceImg.convert(),(sx,sy))
        
        self.init()
        
        while True:
            self.clock.tick(self.speed)
            if self.checkForEvent() == False:
                break
            self.drawAll()
            if self.done == False:
                self.updateBrushStroke()
        self.music.stop()
            








################################################################################
#code below is for running this file standalone to test
def main():
    screen = pygame.display.set_mode((1200, 800))
    class Struct(): pass
    animation = Struct()
    animation.quit = False 
    sourceImg = pygame.image.load('C:\\Users\\Brent\\Desktop\\tpimages\\self.png')
    minStrokeLength = 2
    maxStrokeLength = 8
    threshold       = 50
    listOfBrushes   = [8,4,2]
    bSpline         = True
    blurFactor      = 0.2
    producePainting().run(screen,animation,sourceImg,minStrokeLength,maxStrokeLength,threshold,listOfBrushes,bSpline,blurFactor)
    pygame.quit()

if __name__ == "__main__":
    main()