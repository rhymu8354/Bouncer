using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace Bouncer.Wpf.View {
    public partial class ListBoxExtensions: DependencyObject {
        public static readonly DependencyProperty AutoScrollProperty = DependencyProperty.RegisterAttached(
            "AutoScroll",
            typeof(bool),
            typeof(ListBoxExtensions),
            new UIPropertyMetadata(false, OnAutoScrollChanged)
        );

        public static bool GetAutoScroll(DependencyObject obj) {
            return (bool)obj.GetValue(AutoScrollProperty);
        }

        public static void OnAutoScrollChanged(
            DependencyObject obj,
            DependencyPropertyChangedEventArgs propertyArgs
        ) {
            var listBox = obj as ListBox;
            if (listBox == null) {
                return;
            }
            ScrollViewer scrollViewer = null;
            bool justWheeled = false;
            bool userInteracting = false;
            bool autoScroll = true;
            var collection = listBox.Items.SourceCollection as INotifyCollectionChanged;
            var onScrollChanged = new ScrollChangedEventHandler(
                (scrollChangedSender, scrollChangedArgs) => {
                    if (scrollViewer.VerticalOffset + scrollViewer.ViewportHeight == scrollViewer.ExtentHeight) {
                        autoScroll = true;
                    } else if (justWheeled) {
                        justWheeled = false;
                        autoScroll = false;
                    }
                }
            );
            var onSelectionChanged = new SelectionChangedEventHandler(
                (selectionChangedSender, selectionChangedArgs) => {
                    autoScroll = false;
                }
            );
            var onGotMouse = new MouseEventHandler(
                (gotMouseSender, gotMouseArgs) => {
                    userInteracting = true;
                    autoScroll = false;
                }
            );
            var onLostMouse = new MouseEventHandler(
                (lostMouseSender, lostMouseArgs) => {
                    userInteracting = false;
                }
            );
            var onPreviewMouseWheel = new MouseWheelEventHandler(
                (mouseWheelSender, mouseWheelArgs) => {
                    justWheeled = true;
                }
            );
            var onCollectionChangedEventHandler = new NotifyCollectionChangedEventHandler(
                (collecitonChangedSender, collectionChangedArgs) => {
                    if (
                        (collectionChangedArgs.Action == NotifyCollectionChangedAction.Add)
                        && autoScroll
                        && !userInteracting
                        && (scrollViewer != null)
                    ) {
                        scrollViewer.ScrollToBottom();
                    }
                }
            );
            var hook = new Action(
                () => {
                    if (scrollViewer == null) {
                        scrollViewer = Utilities.FindDescendant<ScrollViewer>(listBox);
                        if (scrollViewer == null) {
                            return;
                        } else {
                            justWheeled = false;
                            userInteracting = false;
                            autoScroll = true;
                            if (scrollViewer != null) {
                                scrollViewer.ScrollToBottom();
                                scrollViewer.ScrollChanged += onScrollChanged;
                            }
                            listBox.SelectionChanged += onSelectionChanged;
                            listBox.GotMouseCapture += onGotMouse;
                            listBox.LostMouseCapture += onLostMouse;
                            listBox.PreviewMouseWheel += onPreviewMouseWheel;
                            collection.CollectionChanged += onCollectionChangedEventHandler;
                        }
                    }
                }
            );
            var unhook = new Action(
                () => {
                    if (scrollViewer != null) {
                        scrollViewer.ScrollChanged -= onScrollChanged;
                        listBox.SelectionChanged -= onSelectionChanged;
                        listBox.GotMouseCapture -= onGotMouse;
                        listBox.LostMouseCapture -= onLostMouse;
                        listBox.PreviewMouseWheel -= onPreviewMouseWheel;
                        collection.CollectionChanged -= onCollectionChangedEventHandler;
                        scrollViewer = null;
                    }
                }
            );
            var onLoaded = new RoutedEventHandler(
                (loadedSender, loadedArgs) => hook()
            );
            var onUnloaded = new RoutedEventHandler(
                (unloadedSender, unloadedArgs) => unhook()
            );
            if ((bool)propertyArgs.NewValue) {
                listBox.Loaded += onLoaded;
                listBox.Unloaded += onUnloaded;
                hook();
            } else {
                listBox.Loaded -= onLoaded;
                listBox.Unloaded -= onUnloaded;
                unhook();
            }
        }

        public static void SetAutoScroll(DependencyObject obj, bool value) {
            obj.SetValue(AutoScrollProperty, value);
        }
    }
}
