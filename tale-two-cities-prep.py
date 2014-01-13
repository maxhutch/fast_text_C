from toolz.curried import *
import urllib
import os

def stem(word):
    """ Stem word to primitive form """
    return word.lower().rstrip(",.!:;'-\"").lstrip("'\"")


filename = 'tale-of-two-cities.txt'

# Get it from the internet if you don't have it already
if not os.path.exists(filename):
    with open(filename, 'w') as f:
        text = urllib.urlopen('http://www.gutenberg.org/ebooks/98.txt.utf-8')
        f.write(text.read())

with open('word-pairs.txt', 'w') as f:
    pipe(filename, open,                # Open file for reading
                   drop(112),           # Drop 112 line header
                   map(str.split),      # Split each line into words
                   concat,              # Join all lists of words to single list
                   map(stem),           # Stem each word
                   sliding_window(2),   # Consider each consecutive pair
                   map(','.join),       # Join each pair with a comma
                   '\n'.join,           # Join all of the pairs with endlines
                   f.write)             # write to file

print pipe(filename, open, set, len)
