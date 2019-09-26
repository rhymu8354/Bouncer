using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;

namespace Bouncer.Wpf.View {
    public class SortAdorner: Adorner {
        #region Public Properties

        public ListSortDirection Direction { get; set; }

        #endregion

        #region Public Methods

        public SortAdorner(UIElement element, ListSortDirection direction)
            : base(element)
        {
            Direction = direction;
        }

        #endregion

        #region UIElement

        protected override void OnRender(DrawingContext drawingContext) {
            base.OnRender(drawingContext);
            if (AdornedElement.RenderSize.Width < 20) {
                return;
            }
            drawingContext.PushTransform(
                new TranslateTransform(
                    AdornedElement.RenderSize.Width - 15,
                    (AdornedElement.RenderSize.Height - 5) / 2
                )
            );
            drawingContext.DrawGeometry(
                Brushes.Black,
                null,
                (
                    (Direction == ListSortDirection.Ascending)
                    ? AscGeometry
                    : DescGeometry
                )
            );
        }

        #endregion

        #region Private Properties

        private static readonly Geometry AscGeometry = Geometry.Parse("M 0 4 L 3.5 0 L 7 4 Z");
        private static readonly Geometry DescGeometry = Geometry.Parse("M 0 0 L 3.5 4 L 7 0 Z");

        #endregion
    }
}
