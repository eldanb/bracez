//
//  MarkerTableCellView.m
//  Bracez
//
//  Created by Eldan Ben Haim on 02/11/2020.
//

#import "MarkerTableCellView.h"
#import "JsonMarker.h"
#import "NodeTypeToColorTransformer.h"

@interface MarkerTableCellView () {
    IBOutlet NSView *view;
    __weak IBOutlet NSTextField *titleView;
}

@end

@implementation MarkerTableCellView

-(instancetype)init {
    self = [super init];
    [self initCommon];
    return self;
}

-(instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    [self initCommon];
    return self;
}

-(instancetype)initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    [self initCommon];
    return self;
}

-(void)layout {
    NSSize viewSize = view.bounds.size;
    [view setFrame:NSMakeRect(0, (self.bounds.size.height - viewSize.height) / 2,
                              self.bounds.size.width, viewSize.height)];
}

-(void)initCommon {
    NSNib *nib = [[NSNib alloc] initWithNibNamed:@"MarkerTableCellView"
                                          bundle:nil];
    
    NSArray* topLevelObjects = nil;
    [nib instantiateWithOwner:self
              topLevelObjects:&topLevelObjects];
    [self addSubview:view];
}

-(void)setObjectValue:(id)objectValue {
    [super setObjectValue:objectValue];
    
    JsonMarker *marker = objectValue;
    switch(marker.markerType) {
        case JsonMarkerTypeError:
            self.imageView.hidden = NO;
            self.imageView.image = [NSImage imageNamed:NSImageNameStatusUnavailable];
            titleView.textColor = nil;
            break;
            
        case JsonMarkerTypeBookmark:
            self.imageView.hidden = NO;
            self.imageView.image = [NSImage imageNamed:NSImageNameStatusAvailable];
            titleView.textColor = nil;
            break;
            
        case JsonMarkerTypeNode:
            self.imageView.hidden = YES;
            titleView.textColor = [[NodeTypeToColorTransformer sharedInstance] colorForNodeType:marker.code];
            break;
    }
}
@end
