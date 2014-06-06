#! /usr/bin/env python

import os
import shutil


SOURCE_DIR = 'BigBenchmarks'
TEMPLATE_DIR = 'BigBenchmarks_templates'
GENERATED_DIR = 'BigBenchmarks_generated'

SOURCE_START = 'opacity: 0'
SOURCE_END = 'opacity: 1'

TEMPLATE_START = '{{START}}'
TEMPLATE_END = '{{END}}'
TEMPLATE_PROPERTY_NAME = '{{PROPERTY_NAME}}'
TEMPLATE_PROPERTY_JAVASCRIPT_NAME = '{{PROPERTY_JAVASCRIPT_NAME}}'
TEMPLATE_PROPERTY_VALUES = '{{PROPERTY_VALUES}}'
TEMPLATE_NUM_ELEMENTS = '{{NUM_ELEMENTS}}'

GENERATED_NUM_ELEMENTS = '1000'

class Variation(object):
    def __init__(self, name, start, end, css_anim, css_transition, web_anim):
        self.name = name
        self.start = start
        self.end = end
        self.property_name = start[0:start.index(':')]
        self.property_values = "'" + start[start.index(':')+2:] + "', '" + end[end.index(':')+2:] + "'"
        self.css_anim = css_anim
        self.css_transition = css_transition
        self.web_anim = web_anim


VARIATIONS = [
    Variation('-compositor-transform', 'transform: rotate(360deg)', 'transform: rotate(0deg)', True, True, True),
    Variation('-mainthread-transform', 'transform: rotate(360deg) translate(1%)', 'transform: rotate(0deg) translate(0%)', True, True, True),
    Variation('-opacity', 'opacity: 0', 'opacity: 1', True, True, True),
    Variation('-px-height', 'height: 20px', 'height: 10px', True, True, True),
    Variation('-em-height', 'height: 8em', 'height: 4em', False, False, True),
    Variation('-inherit-height', 'height: 20px', 'height: inherit', False, False, True),
    Variation('-specific-color', 'background-color: pink', 'background-color: lime', True, True, True),
    Variation('-current-color', 'background-color: pink', 'background-color: currentColor', False, False, True)
]

def optionally_quote_property(property, is_web_anim):
    if not is_web_anim:
        return property
    return property.replace(": ", ": '").replace('-color', 'Color') + "'"

def never_quote_property(property, is_web_anim):
    if not is_web_anim:
        return property
    return property.replace('-color', 'Color')

def prepare():
    try:
        shutil.rmtree(TEMPLATE_DIR)
    except:
        pass
    os.mkdir(TEMPLATE_DIR)

    try:
        shutil.rmtree(GENERATED_DIR)
    except:
        pass
    os.mkdir(GENERATED_DIR)


def read_candidates():
    return os.listdir(SOURCE_DIR)


def is_css_animation(candidate):
    return 'css-animation' in candidate

def is_css_transition(candidate):
    return 'css-transition' in candidate

def is_web_animation(candidate):
    return 'web-animation' in candidate or 'current-time' in candidate


def process(candidate):
    is_web_anim = is_web_animation(candidate)

    with open(SOURCE_DIR + '/' + candidate, 'r') as source_file:
        content = source_file.readlines()

    template_name = TEMPLATE_DIR + '/' + candidate
    with open(template_name, 'w') as template_file:
        for line in content:
            # In web animations, numeric values for properties may or may not be quoted.
            template_file.write(line
                # remove the .0 in set-current-time-request-animation-frame-pow3.html
                .replace(".0'", "'")
                # remove the .4 in css-animations-staggered-triggering-by-inserting-style-element.html
                .replace("0.4;", "0;")
                .replace('var N = 400;', 'var N = '+TEMPLATE_NUM_ELEMENTS+';')
                .replace('var N = 500;', 'var N = '+TEMPLATE_NUM_ELEMENTS+';')
                .replace('var N = 1000;', 'var N = '+TEMPLATE_NUM_ELEMENTS+';')
                .replace(optionally_quote_property(SOURCE_START, is_web_anim), TEMPLATE_START)
                .replace(optionally_quote_property(SOURCE_END, is_web_anim), TEMPLATE_END)
                .replace(SOURCE_START, TEMPLATE_START)
                .replace(SOURCE_END, TEMPLATE_END)
                .replace('transition: opacity', 'transition: '+TEMPLATE_PROPERTY_NAME)
                .replace('style.opacity', 'style.'+TEMPLATE_PROPERTY_JAVASCRIPT_NAME)
                .replace('styleRule.opacity', 'styleRule.'+TEMPLATE_PROPERTY_JAVASCRIPT_NAME)
                .replace(" { opacity: ' + keyframeValues[", " { "+TEMPLATE_PROPERTY_NAME+": ' + keyframeValues[")
                .replace('keyframeValues = [0, 1]', 'keyframeValues = ['+TEMPLATE_PROPERTY_VALUES+']'))

    with open(template_name, 'r') as template_file:
        content = template_file.readlines()

    for variation in VARIATIONS:
        if (variation.css_anim and is_css_animation(candidate)) or (variation.css_transition and is_css_transition(candidate)) or (variation.web_anim and is_web_animation(candidate)):
            # drop -pow3 from set-current-time-request-animation-frame-pow3.html
            generated_name = candidate.replace('-pow3', '').replace('.html', variation.name + '.html')

            # set-current-time-request-animation-frame-pow3.html is currently the only benchmark with pow in its name 
            generated_name = generated_name.replace('-pow3', '')

            generated_num_elements = GENERATED_NUM_ELEMENTS
            # inserting-style-element tests often fail with too many elements, e.g. with px-height changing
            if 'inserting-style-element' in candidate:
                generated_num_elements = '500'
            elif 'animations-many-keyframes' in candidate:
                generated_num_elements = '400'

            # can use jinja2 when requirements become more complex
            with open(GENERATED_DIR + '/' + generated_name, 'w') as generated_file:
                for line in content:
                    generated_file.write(line
                        .replace(TEMPLATE_NUM_ELEMENTS, generated_num_elements)
                        .replace(TEMPLATE_START, optionally_quote_property(variation.start, is_web_anim))
                        .replace(TEMPLATE_END, optionally_quote_property(variation.end, is_web_anim))
                        .replace(TEMPLATE_PROPERTY_NAME, never_quote_property(variation.property_name, is_web_anim))
                        .replace(TEMPLATE_PROPERTY_JAVASCRIPT_NAME, never_quote_property(variation.property_name, True))
                        .replace(TEMPLATE_PROPERTY_VALUES, variation.property_values))

prepare()
candidates = read_candidates()
for candidate in candidates:
    if is_css_animation(candidate) or is_css_transition(candidate) or is_web_animation(candidate):
        process(candidate)
    elif candidate == 'resources':
        shutil.copytree(SOURCE_DIR + '/' + candidate, GENERATED_DIR + '/' + candidate)
    else:
        print('Skipping ' + candidate)
