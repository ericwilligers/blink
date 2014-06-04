#! /usr/bin/env python

import os
import shutil


SOURCE_DIR = 'PerformanceTests/Animation/PerfWeek/BigBenchmarks'
TEMPLATE_DIR = 'PerformanceTests/Animation/PerfWeek/BigBenchmarks_templates'
GENERATED_DIR = 'PerformanceTests/Animation/PerfWeek/BigBenchmarks_generated'

SOURCE_FROM = 'opacity: 0'
SOURCE_TO = 'opacity: 1'

TEMPLATE_FROM = '{{FROM}}'
TEMPLATE_TO = '{{TO}}'

class Variation(object):
    def __init__(self, name, start, end, css_anim, css_transition, web_anim):
        self.name = name
        self.start = start
        self.end = end
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
    Variation('-specific-color', 'color: pink', 'color: lime', True, True, True),
    Variation('-current-color', 'color: pink', 'color: currentColor', False, False, True)
]

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


def process(candidate):

    with open(SOURCE_DIR + '/' + candidate, 'r') as source_file:
        content = source_file.readlines()

    template_name = TEMPLATE_DIR + '/' + candidate
    with open(template_name, 'w') as template_file:
        for line in content:
            template_file.write(line.replace(SOURCE_FROM, TEMPLATE_FROM).replace(SOURCE_TO, TEMPLATE_TO))

    with open(template_name, 'r') as template_file:
        content = template_file.readlines()

    for variation in VARIATIONS:
        if (variation.css_anim and 'css-animation' in candidate) or (variation.css_transition and 'css-transition' in candidate) or (variation.web_anim and 'web-animation' in candidate):
            generated_name = GENERATED_DIR + '/' + candidate.replace('-pow', variation.name + '-pow')
            with open(generated_name, 'w') as generated_file:
                for line in content:
                    generated_file.write(line.replace(TEMPLATE_FROM, variation.start).replace(TEMPLATE_TO, variation.end))

prepare()
candidates = read_candidates()
for candidate in candidates:
    if ('css-animation' not in candidate) and ('css-transition' not in candidate) and ('web-animation' not in candidate):
        print('Skipping ' + candidate)
    else:
        process(candidate)
