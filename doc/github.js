if (document.location.pathname === '/getting-started.html') {
    fetch('https://api.github.com/repos/NeKzor/tem/releases')
        .then((res) => res.json())
        .then((releases) => {
            const addRelease = (id, prerelease) => {
                const release = releases.find((release) => release.prerelease === prerelease);
                const link = document.querySelector(id);
                if (link) {
                    const asset = release?.assets.at(0);
                    link.setAttribute('href', asset?.browser_download_url ?? '#');
                    link.textContent = release?.assets?.at(0)?.name ?? '*not available*';
                }
            };

            addRelease('#stable-release', false);
        })
        .catch(console.error);
}
