/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#import "ViewController.h"

int jscshim_cctest(int argc, char* argv[]);

@interface ViewController ()
    @property (weak, nonatomic) IBOutlet UITextView * console;
    @property NSPipe * stdoutPipe;
    @property NSFileHandle * stdoutPipeReadHandle;
    @property NSThread * nodeThread;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Redirect stdout to the UI
    _console.editable = NO;
    [self readirectStdOut];
    
    _nodeThread = [[NSThread alloc] initWithTarget:self
                                    selector:@selector(nodeThreadMethod:)
                                    object:nil];
    [_nodeThread start];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}
    
/* readirectStdOut and handleNotification are based on https://stackoverflow.com/a/22900278
 * TODO: Close the pipe one shutdown? */
- (void) readirectStdOut {
    _stdoutPipe = [NSPipe pipe];
    _stdoutPipeReadHandle = [_stdoutPipe fileHandleForReading];
    dup2([[_stdoutPipe fileHandleForWriting] fileDescriptor], fileno(stdout));
    
    [[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(handleStdOutNotification:) name: NSFileHandleReadCompletionNotification object: _stdoutPipeReadHandle];
    [_stdoutPipeReadHandle readInBackgroundAndNotify];
}

- (void) handleStdOutNotification:(NSNotification *)notification {
    [_stdoutPipeReadHandle readInBackgroundAndNotify];
    
    NSString * str = [[NSString alloc] initWithData: [[notification userInfo] objectForKey: NSFileHandleNotificationDataItem] encoding: NSUTF8StringEncoding];
    [_console setText:[_console.text stringByAppendingString:str]];
    [_console scrollRangeToVisible:NSMakeRange([_console.text length], 0)];
}

-(void) nodeThreadMethod: (id) sender {
    char * testArgs[] = {"jscshim_cctest", "test-api"};
    jscshim_cctest(2, testArgs);
}

@end
