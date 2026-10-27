#pragma once

#import <Foundation/Foundation.h>

void test() {
     @autoreleasepool {
        NSString *message = @"Hello, Objective-C!";
        NSLog(@"%@", message);
    }
}