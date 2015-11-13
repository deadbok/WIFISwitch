function ToggleHide(id) {
	var element = document.getElementById(id);
	if (element.style.display == 'block')
	{
		element.style.display = 'none';
		element.removeAttribute('style');
	} else {
		element.style.display = 'block';
	}
}
