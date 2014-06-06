function checkWebAnimationsAPI() {
  if (!Element.prototype.animate) {
    // Generate 0 frames and fail if Web Animations is not present.
    document.documentElement.style.display = 'none';
    window.measurementReady = true;
  }
}
