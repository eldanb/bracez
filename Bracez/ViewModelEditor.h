//
//  ViewModelEditor.h
//  Bracez
//
//  Created by Eldan Ben Haim on 17/10/2020.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface ViewModelEditor : NSToolbarItem {
    NSView *view;
}


@property IBOutlet NSObjectController *viewModel;
@property IBOutlet NSView *view;

@end


NS_ASSUME_NONNULL_END
