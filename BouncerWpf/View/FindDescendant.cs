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
using System.Windows.Media;

namespace Bouncer.Wpf.View {
    public static partial class Utilities {
        public static T FindDescendant<T>(Visual root) where T: Visual {
            if (root == null) {
                return null;
            }
            var element = root as T;
            if (element != null) {
                return element;
            }
            (root as FrameworkElement)?.ApplyTemplate();
            int numChildren = VisualTreeHelper.GetChildrenCount(root);
            for (int i = 0; i < numChildren; ++i) {
                var child = VisualTreeHelper.GetChild(root, i) as Visual;
                element = FindDescendant<T>(child);
                if (element != null) {
                    return element;
                }
            }
            return null;
        }
    }
}
