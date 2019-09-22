using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Bouncer.Wpf {

    public class ObservableStdVector<T, E> : IDisposable, IEnumerable, INotifyCollectionChanged where T: IDisposable, IList<E> {
        #region Public Methods

        public ObservableStdVector(T adaptee) {
            Adaptee = adaptee;
        }

        public int IndexOf(E e) {
            return Adaptee.IndexOf(e);
        }

        public void Add(E e) {
            Adaptee.Add(e);
            CollectionChanged?.Invoke(
                this,
                new NotifyCollectionChangedEventArgs(
                    NotifyCollectionChangedAction.Add,
                    e
                )
            );
        }

        public bool Remove(E e) {
            int index = Adaptee.IndexOf(e);
            if (index < 0) {
                return false;
            }
            Adaptee.RemoveAt(index);
            CollectionChanged?.Invoke(
                this,
                new NotifyCollectionChangedEventArgs(
                    NotifyCollectionChangedAction.Remove,
                    e,
                    index
                )
            );
            return true;
        }

        #endregion

        #region IDisposable

        public void Dispose() {
            Dispose(true);
        }

        #endregion

        #region IEnumerable

        public IEnumerator GetEnumerator() {
            return Adaptee.GetEnumerator();
        }

        #endregion

        #region INotifyCollectionChanged

        public event NotifyCollectionChangedEventHandler CollectionChanged;

        #endregion

        #region Protected Methods

        protected virtual void Dispose(bool disposing) {
            if (!Disposed) {
                if (disposing) {
                    Adaptee.Dispose();
                }
                Disposed = true;
            }
        }

        #endregion

        #region Private Properties

        private bool Disposed { get; set; } = false;
        private T Adaptee { get; set; }

        #endregion
    }

}
