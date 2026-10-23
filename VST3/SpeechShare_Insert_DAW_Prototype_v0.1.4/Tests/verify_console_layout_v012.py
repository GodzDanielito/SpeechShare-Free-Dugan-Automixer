"""Static check of the geometry rules used by InsertPluginEditor v0.1.2.
Does not render or compile JUCE, and is not a substitute for host testing.
"""
from math import floor

def check(w,h,source_count,page=0):
    top=max(93,min(132,int(h*93/535)))
    side=max(240,min(430,int(w*0.27)))
    right_x=w-side-9
    viewport_width=max(80,right_x-30)
    viewport_height=max(50,h-top-58)
    available=viewport_width-10
    shown=min(8,max(0,source_count-page*8))
    shown_layout=max(1,shown)
    strip_width=max(78,int((available-(shown_layout-1)*5)/shown_layout))
    needed=shown_layout*strip_width+(shown_layout-1)*5+9
    content_width=max(viewport_width-2,needed)
    content_height=max(345,viewport_height-2)
    strip_h=content_height-3
    met_h=max(85,strip_h-260)
    # Top meter region, fader and group/footer should not overlap
    weight_bottom=111-6+met_h+24
    group_label_top=strip_h-125
    meter_bottom=111+met_h
    assert met_h > 0
    assert weight_bottom <= group_label_top, (w,h,strip_h,weight_bottom,group_label_top)
    assert meter_bottom < group_label_top
    assert content_width>=needed
    assert shown<=8
    # Right panel fits at normal reference: below minimum, viewport scroll is fallback
    right_height=max(50,h-top-29)
    right_content_height=max(405,right_height)
    return dict(editor=f'{w}x{h}',sources=source_count,page=page+1,visible=shown,
                strip_width=strip_width,strip_height=strip_h,strip_scroll=needed>viewport_width,
                right_scroll=right_content_height>right_height)

if __name__=='__main__':
    for w,h in [(760,425),(1020,535),(1200,675),(1600,900),(1900,1100)]:
        for count in [1,2,5,8,16,32]:
            for page in range(max(1,(count+7)//8)):
                result=check(w,h,count,page)
                if count==5 or (count==8 and page==0):
                    print(result)
    print('Static geometry checks passed.')
