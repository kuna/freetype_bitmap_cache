txt = u""

def charset_common():
    global txt
    txt += u'# common\n'
    for i in range(0x0020, 0x007E+1):
        txt += unichr(i)
    txt += u'\n\n'

# korean
def charset_kor():
    global txt
    txt += u'# hangul\n'

    for i in range(0xAC00, 0xD7A3+1):
        txt += unichr(i)
    txt += u'\n'
    for i in range(0x1100, 0x11FF+1):
        txt += unichr(i)
    txt += u'\n'
    for i in range(0x3130, 0x318F+1):
        txt += unichr(i)
    txt += u'\n\n'


# japanese
def charset_jap():
    global txt
    # http://sites.psu.edu/symbolcodes/languages/asia/japanese/katakanachart/
    txt += u'# japanese\n'

    for i in range(0x3000, 0x303F+1):
        txt += unichr(i)
    txt += u'\n'
    for i in range(0x3040, 0x309F+1):
        txt += unichr(i)
    txt += u'\n'
    for i in range(0x30A0, 0x30FF+1):
        txt += unichr(i)
    txt += u'\n'
    """ 
    # AINU part
    for i in range(0x31F0, 0x31FF+1):
        txt += unichr(i)
    txt += u'\n'
    """
    for i in range(0x32D0, 0x32FE+1):
        txt += unichr(i)
    txt += u'\n'
    for i in range(0xFF00, 0xFFEF+1):
        txt += unichr(i)
    txt += u'\n\n'


# japanese with common kanji
def charset_jap_c():
    global txt
    # http://www.rikai.com/library/kanjitables/kanji_codes.unicode.shtml
    txt += u'# japanese with common kanji\n'
    for i in range(0x4e00, 0x9faf+1):
        txt += unichr(i)
    txt += u'\n\n'



charset_common()
charset_kor()
charset_jap()
charset_jap_c()

print 'len about %d' % len(txt)

with open('unicode_charset.txt', 'w') as f:
    f.write(txt.encode('utf-8'))
    f.close()